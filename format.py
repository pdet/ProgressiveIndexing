#!/usr/bin/python

import os

cpp_format_command = 'clang-format -i -sort-includes=${SORT_INCLUDES} -style=file "${FILE}"'
sql_format_command = 'pg_format "${FILE}" -o "${FILE}.out" && mv "${FILE}.out" "${FILE}"'
cmake_format_command = 'cmake-format -i "${FILE}"'
extensions = ['.hpp', '.cpp', '.h']
ignored_files = []

format_commands = {
    '.cpp': cpp_format_command,
    '.c': cpp_format_command,
    '.hpp': cpp_format_command,
    '.h': cpp_format_command,
    '.hh': cpp_format_command,
    '.cc': cpp_format_command,
    '.sql': sql_format_command,
    '.txt': cmake_format_command
}


def format_directory(directory, sort_includes=False):
    directory_printed = False
    files = os.listdir(directory)
    for f in files:
        full_path = os.path.join(directory, f)
        if os.path.isdir(full_path):
            format_directory(full_path, sort_includes)
        else:
            ignored = False
            for ignored_file in ignored_files:
                if ignored_file in full_path:
                    ignored = True
                    break
            if ignored:
                continue
            for ext in extensions:
                if f.endswith(ext):
                    format_command = format_commands[ext]
                    if not directory_printed:
                        print(directory)
                        directory_printed = True
                    cmd = format_command.replace("${FILE}", full_path).replace("${SORT_INCLUDES}",
                                                                               "1" if sort_includes else "0")
                    print(cmd)
                    os.system(cmd)
                    # remove empty lines at beginning and end of file
                    with open(full_path, 'r') as f:
                        text = f.read()
                        text = text.strip() + "\n"
                    with open(full_path, 'w+') as f:
                        f.write(text)
                    break


format_directory('src/', True)
format_directory('test/', True)
