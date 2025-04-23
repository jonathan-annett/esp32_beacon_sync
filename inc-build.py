import re

Import("env")

def increment_build_number(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    build_pattern = re.compile(r'(\bversionBuild\s*:\s*)(\d+)(\s*,?)')
    updated_lines = []
    build_found = False

    for line in lines:
        match = build_pattern.search(line)
        if match:
            build_number = int(match.group(2)) + 1
            new_line = build_pattern.sub(
                lambda m: f"{m.group(1)}{build_number}{m.group(3)}", line
            )
            updated_lines.append(new_line)
            build_found = True
        else:
            updated_lines.append(line)

    if not build_found:
        raise ValueError("versionBuild not found in file")

    with open(file_path, 'w') as f:
        f.writelines(updated_lines)

    print(f"versionBuild incremented to {build_number}")


def before_build(source, target, env):
     increment_build_number("src/version.h")




 
increment_build_number("src/version.h")
 
 