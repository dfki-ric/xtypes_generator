#!python3

def can_be_used():
    return True


def cant_be_used_msg():
    return "Unknown error!"


INFO = 'Performs the processes of get_files and copy_files commands at once.'


def main(args):
    import argparse
    parser = argparse.ArgumentParser(
        description='Gets the list of files and copies the files found to the given output dir')
    parser.add_argument('--skeleton_dir', help="The output directory where to put the skeleton that the user can edit."
                                               "Only for cpp. A include/ and src/ directories will be created in the "
                                               "given directory",
                        type=str, default=None)
    parser.add_argument('--user_dir', help="The source dir of the user which contains include/ and src/",
                        type=str, default=None)
    parser.add_argument('--files', help="Returns the list of files",
                        choices=["USER_HEADERS", "USER_SOURCES", "USER",
                                 "SKELETON_HEADERS", "SKELETON_SOURCES", "SKELETON",
                                 "HEADERS", "SOURCES", "ALL", "HEADERS_SOURCES"],
                        default=None)
    parser.add_argument('--output_dir', help="The output directory where to copy the given files", type=str,
                        default=None)
    parser.add_argument('-v', '--verbose', help="Print the path of the files in output_dir", action="store_true",
                        default=False)
    args = parser.parse_args(args)

    from ..file_handling import get_files, copy_files

    files = get_files(return_list=True, args=args)
    copy_files(files=files, output_dir=args.output_dir, verbose=args.verbose)


if __name__ == "__main__":
    import sys
    main(sys.argv)
