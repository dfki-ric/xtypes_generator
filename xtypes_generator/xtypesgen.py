#!python
try:
    # in newer version this deprecates in favor of importlib.resources
    from importlib.metadata import version, PackageNotFoundError
except ImportError:
    try:
        from importlib_metadata import version, PackageNotFoundError
    except ImportError:
        from pkg_resources import get_distribution, DistributionNotFound as PackageNotFoundError
        def version(package_name):
            return get_distribution("xypes_generator").version
try:
    __version__ = version("xtypes_generator")
except PackageNotFoundError:
    __version__ = '1.0.0'


def main():
    import sys
    import os
    try:
        from . import scripts
    except ImportError:
        try:
            from xtypes_generator import scripts
        except ImportError as e:
            print("PYTHONPATH", os.environ["PYTHONPATH"], flush=True)
            raise e

    script_files = [f for f in dir(scripts) if not f.startswith("__")]
    available_scripts = [(f, getattr(scripts, f).INFO, None) for f in script_files if getattr(scripts, f).can_be_used()]
    unavailable_scripts = [(f, getattr(scripts, f).INFO, getattr(scripts, f).cant_be_used_msg()) for f in script_files
                           if not getattr(scripts, f).can_be_used()]

    if len(sys.argv) > 1 and sys.argv[1] in [ascr[0] for ascr in available_scripts + unavailable_scripts]:
        if sys.argv[1] in unavailable_scripts:
            print("Attention: Script might not work properly:", getattr(scripts, sys.argv[1]).cant_be_used_msg())
        getattr(scripts, sys.argv[1]).main(sys.argv[2:])
        return 0
    else:
        print(sys.argv)
        print("This tool is here to support you with your XTypes Project"+"\n")
        print("Usage:")
        print("xtypes_generator COMMAND ARGUMENTS")
        print("Commands:")
        spaces = 0
        for script in available_scripts + unavailable_scripts:
            if len(script[0]) > spaces:
                spaces = len(script[0])
        spaces += 3
        for script, info, _ in sorted(available_scripts):
            print("       "+script+" "*(spaces-len(script))+info)
        for script, info, error in unavailable_scripts:
            print("     X "+script+" "*(spaces-len(script))+info+"  UNAVAILABLE: "+error)
        return 1


if __name__ == "__main__":
    main()
