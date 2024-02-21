#!python3

def can_be_used():
    return True


def cant_be_used_msg():
    return "Unknown error!"


INFO = 'Sets up the directory structure for a new XTypes project and initiates it.'


def main(args):
    import argparse
    import os
    try:
        # Try importing from importlib.resources (Python 3.7+)
        from importlib.resources import files as import_resources_files
    except ImportError:
        try:
            # Try importing from importlib_resources (Python 3.9+)
            from importlib_resources import files as import_resources_files
        except ImportError:
            # For versions older than Python 3.8, fallback to pkg_resources
            import pkg_resources

    parser = argparse.ArgumentParser(
        description='Copies the given files to the specified output')
    parser.add_argument('--project_name', help="The name of the project. Will also be the dir name.", type=str, nargs="+", default=None)
    parser.add_argument('--version', help="The version of the project.", type=str, nargs="+", default=None)
    parser.add_argument('--description', help="The version of the project.", type=str, nargs="+", default=None)
    parser.add_argument('-n', '--non-interactive', help="The version of the project.", action="store_true", default=False)
    args = parser.parse_args(args)

    if not args.non_interactive:
        if args.project_name is None:
            args.project_name = input("Please give the name for the project (avoid special characters and white spaces, _ is ok): ").strip().replace(" ", "_").replace("-", "_")
            assert len(args.project_name.strip()) > 0 and args.project_name.strip() not in [".", ".."] and "/" not in args.project_name.strip()
        if args.version is None:
            args.version = input("Please give the version for the project: ").strip()
        if args.description is None:
            args.description = input("Please give a short description for the project: ").strip()
            if args.description[0] not in ["'", '"']:
                if '"' in args.description:
                    args.description = args.description.replace('"', "'")
                args.description = f'"{args.description}"'

    assert args.project_name is not None

    os.makedirs(args.project_name, exist_ok=True)
    os.makedirs(os.path.join(args.project_name, "include"), exist_ok=True)
    os.makedirs(os.path.join(args.project_name, "src"), exist_ok=True)
    os.makedirs(os.path.join(args.project_name, "templates"), exist_ok=True)
    os.makedirs(os.path.join(args.project_name, "cmake"), exist_ok=True)


    try:
        cmakelists_path = import_resources_files("xtypes_generator").joinpath("data/CMakeLists.txt.in")
    except NameError:
        cmakelists_path = pkg_resources.resource_filename("xtypes_generator", "data/CMakeLists.txt.in")

    with open(cmakelists_path, "r") as in_file:
        with open(os.path.join(args.project_name, "CMakeLists.txt"), "w") as out_file:
            text = in_file.read()

            text = text.replace("@PROJECT_NAME@", args.project_name)
            if args.version is not None:
                text = text.replace("@VERSION@", "VERSION "+args.version)
            else:
                text = text.replace("@VERSION@", "")
            if args.description is not None:
                text = text.replace("@DESCRIPTION@", "DESCRIPTION "+args.description)
            else:
                text = text.replace("@DESCRIPTION@", "")

            out_file.write(text)

    try:
        cmakeconfig_path = import_resources_files("xtypes_generator").joinpath("data/config.cmake.in")
    except NameError:
        cmakeconfig_path = pkg_resources.resource_filename("xtypes_generator", "data/config.cmake.in")

    with open(cmakeconfig_path, "r") as in_file:
        with open(os.path.join(args.project_name, "cmake", args.project_name+"-config.cmake.in"), "w") as out_file:
            text = in_file.read()

            text = text.replace("@PROJECT_NAME@", args.project_name)
            if args.version is not None:
                text = text.replace("@VERSION@", "VERSION "+args.version)
            else:
                text = text.replace("@VERSION@", "")
            if args.description is not None:
                text = text.replace("@DESCRIPTION@", "DESCRIPTION "+args.description)
            else:
                text = text.replace("@DESCRIPTION@", "")

            out_file.write(text)

    #os.chdir(os.path.join(args.project_name))
    #os.makedirs(os.path.join(args.project_name, "build"), exist_ok=True)
    # import shutil
    # skeletons_path = os.path.join(args.project_name, "build", "auto_generated_files", "skeleton_files", "src")
    # for f in os.listdir(skeletons_path):
    #     if not os.path.isfile(os.path.join(args.project_name, "src")):
    #         shutil.copy(os.path.join(skeletons_path, f), os.path.join(args.project_name, "src", f))


if __name__ == "__main__":
    import sys
    main(sys.argv)
