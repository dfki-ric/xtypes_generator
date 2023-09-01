#!python3

def can_be_used():
    return True


def cant_be_used_msg():
    return "Unknown error!"


INFO = 'Gets the list of file pathes to the specifed (generated) files.'


def main(args):
    import argparse
    parser = argparse.ArgumentParser(
        description='Provides the list of headers and sources that takes the users files if present and the '
                    'skeleton files if not.')
    parser.add_argument('--skeleton_dir', help="The output directory where to put the skeleton that the user can "
                                               "edit. Only for cpp. A include/ and src/ directories will be "
                                               "created in the given directory",
                        type=str, default=None)
    parser.add_argument('--user_dir', help="The source dir of the user which contains include/ and src/",
                        type=str, default=None)
    parser.add_argument('--files', help="Returns the list of files",
                        choices=["USER_HEADERS", "USER_SOURCES", "USER",
                                 "SKELETON_HEADERS", "SKELETON_SOURCES", "SKELETON",
                                 "HEADERS", "SOURCES", "ALL", "HEADERS_SOURCES"],
                        default=None)
    args = parser.parse_args(args)

    from ..file_handling import get_files

    get_files(args=args, return_list=False)


if __name__ == "__main__":
    import sys
    main(sys.argv)
