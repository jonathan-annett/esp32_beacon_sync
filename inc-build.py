import re
import json
from SCons.Script import Import

Import("env")

def increment_build_number(file_path, json_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    build_pattern = re.compile(r'(\bversionBuild\s*:\s*)(\d+)(\s*,?)')
    major_pattern = re.compile(r'(\bversionMajor\s*:\s*)(\d+)(\s*,?)')
    minor_pattern = re.compile(r'(\bversionMinor\s*:\s*)(\d+)(\s*,?)')

    updated_lines = []
    major = minor = build = None

    for line in lines:
        if (m := major_pattern.search(line)):
            major = int(m.group(2))
        if (m := minor_pattern.search(line)):
            minor = int(m.group(2))
        if (m := build_pattern.search(line)):
            build = int(m.group(2)) + 1
            line = build_pattern.sub(
                lambda m: f"{m.group(1)}{build}{m.group(3)}", line
            )
        updated_lines.append(line)

    if build is None:
        raise ValueError("versionBuild not found in file")

    with open(file_path, 'w') as f:
        f.writelines(updated_lines)

    print(f"versionBuild incremented to {build}")

    # Write to JSON
    version_info = {
        "versionMajor": major,
        "versionMinor": minor,
        "versionBuild": build
    }
    with open(json_path, 'w') as f:
        json.dump(version_info, f, indent=4)

    print(f"Version info written to {json_path}")

# Run immediately on import
increment_build_number("src/version.h", "src/version.json")
