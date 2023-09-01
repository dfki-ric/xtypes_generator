import os
import argparse
import shutil


def get_files(args, return_list=False):
    user_headers = os.listdir(os.path.join(args.user_dir, "include")) if os.path.exists(os.path.join(args.user_dir, "include")) else []
    user_sources = os.listdir(os.path.join(args.user_dir, "src")) if os.path.exists(os.path.join(args.user_dir, "src")) else []

    skel_headers = os.listdir(os.path.join(args.skeleton_dir, "include")) if os.path.exists(os.path.join(args.skeleton_dir, "include")) else []
    skel_sources = os.listdir(os.path.join(args.skeleton_dir, "src")) if os.path.exists(os.path.join(args.skeleton_dir, "src")) else []

    headers = []
    sources = []
    if "SKELETON" not in args.files:
        headers = [os.path.abspath(os.path.join(args.user_dir, "include", f)) for f in user_headers]
        sources = [os.path.abspath(os.path.join(args.user_dir, "src", f)) for f in user_sources]
    if "USER" not in args.files:
        headers += [os.path.abspath(os.path.join(args.skeleton_dir, "include", f))
                    for f in skel_headers if f not in user_headers]
        sources += [os.path.abspath(os.path.join(args.skeleton_dir, "src", f))
                    for f in skel_sources if f not in user_sources]

    if "HEADERS" in args.files and "SOURCES" in args.files or not ("HEADERS" in args.files or "SOURCES" in args.files):
        if return_list:
            return headers + sources
        else:
            print("\n".join(headers))
            print("\n".join(sources))
    elif "HEADERS" in args.files:
        if return_list:
            return headers
        else:
            print("\n".join(headers))
    elif "SOURCES" in args.files:
        if return_list:
            return sources
        else:
            print("\n".join(sources))


def copy_files(files=None, output_dir=None, verbose=False):
    assert files is not None and output_dir is not None
    os.makedirs(output_dir, exist_ok=True)
    for of in os.listdir(output_dir):
        if of not in [os.path.basename(f).strip() for f in files]:
            os.remove(os.path.join(output_dir, of))
    for f in files:
        name = os.path.basename(f).strip()
        of_content = None
        if name in os.listdir(output_dir):
            of_content = open(os.path.join(output_dir, name)).read()
        f_content = open(f).read()
        if of_content != f_content:
            shutil.copyfile(f, os.path.join(output_dir, name))
        if verbose:
            print(os.path.abspath(os.path.join(output_dir, name)), end=";")
