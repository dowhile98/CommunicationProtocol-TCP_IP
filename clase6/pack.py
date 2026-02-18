#!/usr/bin/env python3
# Uso por defecto (sin especificar tamaño):

# Bash

# python pack_rc.py web_root compressed build/packed_fs.c
# Se usará el tamaño máximo de 1 MB.

# Especificando un tamaño máximo de 2 MB:

# Bash

# python pack_rc.py web_root compressed build/packed_fs.c --maxsize 2097152
# Se usará el tamaño máximo de 2097152 bytes (2 MB).

# Con la opción de no comprimir y un tamaño máximo de 500 KB:

# Bash

# python pack_rc.py web_root compressed build/no_gzip.c -n -m 512000
# Se usará web_root como origen, sin compresión, y el tamaño máximo será de 512000 bytes.

import os
import sys
import struct
import gzip
import argparse

RES_TYPE_DIR = 1
RES_TYPE_FILE = 2

TRESENTRY_BASE_SIZE = 1 + 4 + 4 + 1
TRESHEADER_SIZE = 4 + TRESENTRY_BASE_SIZE

def align4(x):
    return ((x + 3) // 4) * 4

class ResourceCompiler:
    def __init__(self, max_size=1024*1024):
        self.max_size = max_size
        self.buf = bytearray(max_size)
        self.total_size = TRESHEADER_SIZE
        self._write_u32(0, self.total_size)
        self._write_entry(4, RES_TYPE_DIR, self.total_size, 0, b'')

    def _ensure_space(self, needed):
        if self.total_size + needed > self.max_size:
            raise MemoryError("Maximum size exceeded")

    def _write_u8(self, off, val):
        self.buf[off:off+1] = struct.pack('<B', val)

    def _write_u32(self, off, val):
        self.buf[off:off+4] = struct.pack('<I', val)

    def _write_bytes(self, off, bts):
        self.buf[off:off+len(bts)] = bts

    def _write_entry(self, off, type_, dataOffset, dataLength, name_bytes):
        name_len = len(name_bytes)
        self._write_u8(off + 0, type_)
        self._write_u32(off + 1, dataOffset)
        self._write_u32(off + 5, dataLength)
        self._write_u8(off + 9, name_len)
        if name_len:
            self._write_bytes(off + TRESENTRY_BASE_SIZE, name_bytes)

    def add_file_contents(self, filename):
        with open(filename, 'rb') as f:
            f.seek(0, os.SEEK_END)
            length = f.tell()
            f.seek(0)
            if self.total_size + length > self.max_size:
                raise MemoryError("Maximum size exceeded")
            data = f.read()
            self._write_bytes(self.total_size, data)
            offset = self.total_size
            self.total_size += length
            print(f"[FILE]    {filename}    ({length} bytes)")
            return offset, length

    def add_directory(self, parentOffset, parentSize, directory):
        pos = self.total_size
        i = pos
        dir_length = 0

        print(f"[DIR]     {directory}")

        self._ensure_space(TRESENTRY_BASE_SIZE + 1)
        self._write_entry(i, RES_TYPE_DIR, 0, 0, b'.')
        i += TRESENTRY_BASE_SIZE + 1
        dir_length += TRESENTRY_BASE_SIZE + 1

        if parentOffset and parentSize:
            self._ensure_space(TRESENTRY_BASE_SIZE + 2)
            self._write_entry(i, RES_TYPE_DIR, 0, 0, b'..')
            i += TRESENTRY_BASE_SIZE + 2
            dir_length += TRESENTRY_BASE_SIZE + 2

        entries = sorted(os.listdir(directory))
        for name in entries:
            if name in ('.', '..'):
                continue
            fullpath = os.path.join(directory, name)
            entry_type = RES_TYPE_DIR if os.path.isdir(fullpath) else RES_TYPE_FILE
            name_bytes = name.encode('utf-8')
            self._ensure_space(TRESENTRY_BASE_SIZE + len(name_bytes))
            self._write_entry(i, entry_type, 0, 0, name_bytes)
            i += TRESENTRY_BASE_SIZE + len(name_bytes)
            dir_length += TRESENTRY_BASE_SIZE + len(name_bytes)

        self.total_size += dir_length

        read_ptr = pos
        remaining = dir_length
        while remaining > 0:
            type_ = struct.unpack_from('<B', self.buf, read_ptr)[0]
            nameLength = struct.unpack_from('<B', self.buf, read_ptr + 9)[0]
            name_bytes = bytes(self.buf[read_ptr + TRESENTRY_BASE_SIZE: read_ptr + TRESENTRY_BASE_SIZE + nameLength])
            name = name_bytes.decode('utf-8') if nameLength else ''

            if name == '.':
                self._write_u32(read_ptr + 1, pos)
                self._write_u32(read_ptr + 5, dir_length)
            elif name == '..':
                self._write_u32(read_ptr + 1, parentOffset)
                self._write_u32(read_ptr + 5, parentSize)
            else:
                aligned = align4(self.total_size)
                if aligned != self.total_size:
                    pad_len = aligned - self.total_size
                    self._ensure_space(pad_len)
                    self.total_size = aligned

                self._write_u32(read_ptr + 1, self.total_size)
                fullpath = os.path.join(directory, name)
                if type_ == RES_TYPE_DIR:
                    sub_offset, sub_len = self.add_directory(pos, dir_length, fullpath)
                    self._write_u32(read_ptr + 5, sub_len)
                else:
                    off, length = self.add_file_contents(fullpath)
                    self._write_u32(read_ptr + 5, length)

            step = TRESENTRY_BASE_SIZE + nameLength
            remaining -= step
            read_ptr += step

        return pos, dir_length

    def finalize_and_write(self, dest_file):
        self._write_u32(0, self.total_size)
        ext = os.path.splitext(dest_file)[1].lower()
        if ext in ('.c', '.h'):
            os.makedirs(os.path.dirname(dest_file) or '.', exist_ok=True)
            base = os.path.splitext(os.path.basename(dest_file))[0]
            with open(dest_file, 'w', newline='\n') as f:
                f.write(f"#include \"tcs_xxx_config.h\"\n\r#include <stdint.h>\n\r\n\rEXTFLASH_MEM_ATTRIBUTE const uint8_t {base}[] =\n{{\n")
                for i in range(self.total_size):
                    if (i % 16) == 0:
                        f.write("    ")
                    f.write(f"0x{self.buf[i]:02X}")
                    if i == (self.total_size - 1):
                        f.write("\n")
                    elif (i % 16) == 15:
                        f.write(",\n")
                    else:
                        f.write(", ")
                f.write("};\n")
        else:
            os.makedirs(os.path.dirname(dest_file) or '.', exist_ok=True)
            with open(dest_file, 'wb') as f:
                f.write(self.buf[:self.total_size])

def compress_files(input_dir, output_dir):
    try:
        if not os.path.isdir(input_dir):
            print(f"Error: El directorio de entrada '{input_dir}' no existe.")
            return False

        os.makedirs(output_dir, exist_ok=True)

        print("____________________________________________________________")
        print("Comprimiendo archivos...")
        print("pack_rc.py - Quino B. Jeffry")
        print("____________________________________________________________")

        for dirpath, dirnames, filenames in os.walk(input_dir):
            for filename in filenames:
                file_path = os.path.join(dirpath, filename)
                relative_path = os.path.relpath(file_path, input_dir)
                
                with open(file_path, 'rb') as f:
                    file_data = f.read()

                compressed_data = gzip.compress(file_data)

                compressed_file_path = os.path.join(output_dir, relative_path) + '.gz'
                os.makedirs(os.path.dirname(compressed_file_path), exist_ok=True)
                
                with open(compressed_file_path, 'wb') as cf:
                    cf.write(compressed_data)

                print(f"Comprimido: {relative_path}.gz [{len(compressed_data)} bytes]")
                print("------------------------------------------------------------")
        
        print("Proceso de compresión finalizado.\n")
        return True

    except Exception as e:
        print(f"Ocurrió un error durante la compresión: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Empaqueta archivos estáticos y los compila en un archivo C.")
    
    parser.add_argument('input_dir', type=str, nargs='?', default='dist',
                        help="Carpeta raíz de los archivos originales (por defecto: 'dist').")
    
    parser.add_argument('compressed_dir', type=str, nargs='?', default='resources',
                        help="Carpeta donde se guardarán los archivos comprimidos (por defecto: 'resources').")
                        
    parser.add_argument('output_file', type=str, nargs='?', default='res.c',
                        help="Ruta del archivo de salida .c (por defecto: 'res.c').")
    
    parser.add_argument('-n', '--no-compress', action='store_true',
                        help="No comprimir los archivos, utilizar la carpeta de entrada directamente.")

    parser.add_argument('-m', '--maxsize', type=int, default=1024*1024,
                        help="Tamaño máximo del archivo de salida en bytes (por defecto: 1048576).")

    args = parser.parse_args()

    source_dir_for_compiler = args.input_dir

    if not args.no_compress:
        if compress_files(args.input_dir, args.compressed_dir):
            source_dir_for_compiler = args.compressed_dir
    else:
        print("Saltando la etapa de compresión. Utilizando la carpeta de entrada directamente.")
    
    if not os.path.isdir(source_dir_for_compiler):
        print(f"Error: El directorio de origen para la compilación '{source_dir_for_compiler}' no existe.")
        return 1

    rc = ResourceCompiler(max_size=args.maxsize)
    try:
        path = os.path.abspath(source_dir_for_compiler)
        pos, length = rc.add_directory(0, 0, path)
        rc._write_u32(4 + 5, length)
        rc.finalize_and_write(args.output_file)
    except MemoryError as e:
        print("Error:", e)
        return 1
    except Exception as e:
        print("Unhandled error:", e)
        return 1

    print("---------------------------------------------------------\r\n")
    print("Resource compilation completed successfully")
    print(f"{rc.total_size} bytes successfully written to {args.output_file} !")
    print(f"Resource directory: {source_dir_for_compiler}")
    print(f"Output file: {args.output_file}")
    print(f"maximum size: {args.maxsize} bytes")
    print(f"free space: {args.maxsize - rc.total_size} bytes")
    print("---------------------------------------------------------\r\n")
    return 0

if __name__ == '__main__':
    sys.exit(main())
