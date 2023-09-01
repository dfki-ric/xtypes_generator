#!python3

def can_be_used():
    return True


def cant_be_used_msg():
    return "Unknown error!"


INFO = 'Copies the list of given files to the output directory.'


def main(args):
    import argparse
    parser = argparse.ArgumentParser(
        description='Copies the given files to the specified output')
    parser.add_argument('--files', help="The files to copy", type=str, nargs="+", default=None)
    parser.add_argument('--output_dir', help="The output directory where to copy the given files", type=str,
                        default=None)
    parser.add_argument('-v', '--verbose', help="Print the path of the files in output_dir", action="store_true",
                        default=False)
    args = parser.parse_args(args)

    from ..file_handling import copy_files

    copy_files(files=args.files, output_dir=args.output_dir, verbose=args.verbose)


if __name__ == "__main__":
    import sys
    main(sys.argv)
