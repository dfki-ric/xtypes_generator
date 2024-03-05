#!python3

def can_be_used():
    return True


def cant_be_used_msg():
    return "Unknown error!"


INFO = 'Code generator script that takes a xtype yaml template and converts it to C++ class data type.'


import datetime
from enum import Enum
import sys
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

import yaml
import os
from jinja2 import Environment, PackageLoader, select_autoescape


# Loads a yaml file & returns its content
def load_yaml_file(filename):
    """
    Load the given yaml file. Excepts when there is a YAMLError.
    :param filename:
    :return:
    """
    try:
        with open(filename, "r") as file:
            data = yaml.safe_load(file)
            return data
    except yaml.YAMLError as exc:
        print(exc)
        exit(1)


def create_dir(directory):
    """
    Creates the specified directory. Excepts when there is an OSError during creation
    :param directory: directory to create
    :return: the given directory
    """
    try:
        os.makedirs(directory, exist_ok=True)
    except OSError:
        print('Error: creating directory. ' + directory)
        exit(1)
    return directory


# Supported languages
class Language(Enum):
    CPP = 0,
    PYTHON = 1

    def __str__(self):
        return str(self.name.upper())


# Load type conversion info from package
# TODO: Allow this to be overridden by CLI
# TODO: Make conversion table per template section ('properties', 'relations' ...) ?
# TODO: This functionality has to be revised thoroughly!!!!!! It should be supported/generalized more
try:
    TypeConversion = load_yaml_file(import_resources_files("xtypes_generator").joinpath("data/type_conversion.yml"))
except NameError:
    TypeConversion = load_yaml_file(pkg_resources.resource_filename("xtypes_generator", "data/type_conversion.yml"))

JsonTypes = {
#"null": "nl::json::value_t::null",
"BOOLEAN": "nl::json::value_t::boolean",
"STRING": "nl::json::value_t::string",
"INTEGER": "nl::json::value_t::number_integer",
"INTEGER64": "nl::json::value_t::number_integer",
#"INTEGER": "nl::json::value_t::number_unsigned",
"FLOAT": "nl::json::value_t::number_float",
"FLOAT64": "nl::json::value_t::number_float",
# FIXME: aggregate types below are not resolved properly!!!
"JSON": "nl::json::value_t::object",
"MAP": "nl::json::value_t::object",
"VECTOR": "nl::json::value_t::array",
"SET": "nl::json::value_t::array",
"ENUM": "nl::json::value_t::number_integer",
#"binary": "nl::json::value_t::binary",
#"discarded": "nl::json::value_t::discarded",
}
# Converts PropertyType string identifier to a string representation of the desired language's data type
def parse_type(property_type: str, lang: Language) -> tuple:
    """
    Parses the type string that is specified in the Yaml Template file for an XType
    :param property_type: the type string
    :param lang: the language to parse the type for
    :return: tuple that contains as first element the type string in the specified language, as second element the
      template type strings when parsing a template type string
    """
    property_type = property_type.strip()
    if property_type.startswith("TEMPLATE"):
        template_types = TypeConversion["default_supported_template_types"]
        if "[" in property_type:
            if property_type[8] == ":":
                template_types = [t.strip() for t in property_type[11:-1].split(";")]
            else:
                template_types = [t.strip() for t in property_type[9:-1].split(";")]
        return TypeConversion["TEMPLATE"][str(lang)], template_types
    elif property_type.startswith("XTYPE"):
        return property_type[6:-1], None
    elif property_type.startswith("FUNCTION"):
        inner_spec = property_type[property_type.find("(") + 1: property_type.rfind(")")]
        result = "std::function< "+inner_spec+" >"
        return result, None
    # FIXME: This is a dangerous elif statement when an unknown identifier contains (
    elif "(" in property_type:
        template_args = property_type[property_type.find("(") + 1:-1].replace(" ", "")
        n_par = 0
        args = []
        _temp = ""
        for x in template_args:
            if x == "(":
                n_par += 1
            if n_par == 0 and x == ",":
                args += [_temp]
                _temp = ""
                continue
            else:
                _temp += x
            if x == ")":
                n_par -= 1
        args += [_temp]
        base_type = property_type[:property_type.find("(")].upper()
        if base_type.startswith("SET") and args[0].startswith("XTYPE"):
            raise TypeError(f"You can't use XType pointers in std::set, as this will lead to undefined behavior.\n(type was: {property_type})")
        property_type = parse_type(base_type, lang)[0]
        for arg, t in zip(args, TypeConversion[base_type]["template_args"]):
            property_type = property_type.replace("@" + t + "@", parse_type(arg, lang)[0])
        return property_type, None
    elif property_type.upper() not in TypeConversion.keys():
        return property_type, None
    else:
        return TypeConversion[property_type.upper()][str(lang)], None


# Make language specific adjustments to default values given the type of the property
# TODO: Perform value check. Maybe like this:
#  pytype = parse_type(ptype_in, PYTHON), pdefault = globals()[pytype](pdefault)
def parse_default(default, prop_type, lang, function_argument=False):
    """
    Parses the default value for an argument in an XType method from the template yaml. And returns it in the specified
    Language
    :param default: the default value from the yaml file
    :param prop_type: the argument type
    :param lang: the language
    :return:
    """
    default_value = default
    if lang == Language.CPP:
        # if prop_type.upper().startswith("VECTOR"):
        #    default_value = repr(default).replace("[", "{").replace("]", "}")
        if prop_type.upper() == "BOOLEAN":
            default_value = repr(default).lower()
        elif function_argument and prop_type.upper() == "JSON" and ((type(default) == str and default.startswith("{")) or (type(default) == dict and len(default) == 0)):
            default_value = "nl::json(nl::json::value_t::object)"
    return default_value


def parse_allowed(allowed, prop_type, lang):
    result = set()
    for val in allowed:
        result.add(parse_default(val, prop_type, lang))
    return result


def get_xtype(property_type: str) -> (str, None):
    """Takes a property_type string and returns the basic xtype name"""
    if "XTYPE" in property_type:
        lst_prt = property_type[property_type.find("XTYPE(")+6:]
        if lst_prt.startswith("Const") and lst_prt[5].isupper():
            lst_prt = lst_prt[5:]
        return lst_prt[:lst_prt.find(")")].replace("CPtr", "").replace("Ptr", "")
    return None


def is_xtype(property_type: str) -> bool:
    """
    Checks whether the spcified type is a XType type
    :param property_type: the string specified in the XType template yaml
    :return: boolean
    """
    return property_type.startswith("XTYPE")


def is_template(property_type: str) -> (bool, int):
    """
    Checks whether the spcified type is a Template type
    :param property_type: the string specified in the XType template yaml
    :return: boolean
    """
    if property_type.startswith("TEMPLATE:"):
        return int(property_type[10])
    return property_type.startswith("TEMPLATE")

# Parse the yaml files and collect all necessary info for the template engine
def parse_yaml(yaml_data, project_name, lang: Language):
    """
    Parses the yaml from the template yaml files and extracts the information in such way that it can be passed to the
    jinja templates
    :param yaml_data: a dict from the Xtype template file
    :param lang: the language to parse this for
    :return: jinja ready information tuple: (classname, properties, relations, sorted(classes), custom_uri, methods,
      template_types, inherit)
    """
    classname = yaml_data["name"]
    inherit = None if "inherit" not in yaml_data else yaml_data["inherit"]
    properties = {}
    default_template_types = []
    if "properties" in yaml_data:
        for top_key, top_value in yaml_data["properties"].items():
            # If 'type' is not in top_value keys, we have to recursively resolve all subkeys until we find a 'type'.
            # For each path from top_key to some 'type' we have to call the code below! That means that we need some form of stack (to not use recursive calls)
            unresolved = [(top_key, top_value)]
            resolved = []
            while len(unresolved) > 0:
                current_key, current_value = unresolved.pop(0)
                if "type" not in current_value:
                    for k,v in current_value.items():
                        unresolved.append((current_key + "/" + k, v))
                    continue
                # Found leaf property
                resolved.append((current_key, current_value))
            # For each resolved key, value pair, we create a property entry
            for pname, prop in resolved:
                try:
                    if pname in properties:
                        raise ValueError(f"Property {pname} already exists in {classname}")
                    ptype_in = prop["type"]
                    if is_xtype(ptype_in):
                        raise ValueError(f"Type {ptype_in} not allowed in property {pname} in {classname}. Use relations!")
                    pdefault = ""
                    if "default" in prop:
                        pdefault = parse_default(prop["default"], ptype_in, lang)
                    pallowed = set()
                    if "allowed" in prop:
                        pallowed = list(parse_allowed(prop["allowed"], ptype_in, lang))
                    # TODO: Since we use json types only, we should modify this function to return the proper JSON type in the given language
                    # However, currently we use convenient getter/setter functions which actually use the correct C++ types
                    ptype, ptempl = parse_type(ptype_in, lang)
                    if ptype.startswith("XType") and not "::" in ptype:
                        ptype = project_name+"::"+ptype
                    if ptempl is not None:
                        default_template_types += ptempl
                    properties[pname] = (JsonTypes[ptype_in.split("(")[0].upper()] if ptype_in.split("(")[0].upper() in JsonTypes else None, sorted(pallowed), pdefault, ptype, "advanced_setter" in prop and prop["advanced_setter"])
                except:
                    raise RuntimeError(f"Error in property definition of {classname}::{pname}: {prop}")
    relations = {}
    classes = set()
    if "relations" in yaml_data:
        for attrname, rel in yaml_data["relations"].items():
            try:
                if attrname in relations:
                    raise ValueError(f"Relation attribute {attrname} already exists in relations in {classname}.")
                if attrname in properties:
                    raise ValueError(f"Relation attribute {attrname} already exists in properties in {classname}.")
                rname = rel["type"].upper()
                rclassnames_raw = set()
                if "other_classname" in rel:
                    print(f"WARNING: Found old specification of 'other_classname' in {project_name}::{classname}. Please use 'other_classnames' instead.", file=sys.stderr)
                    rclassnames_raw.add(rel["other_classname"])
                else:
                    rclassnames_raw = set(rel["other_classnames"])
                rclassnames = set()
                for cname in rclassnames_raw:
                    classes.add(cname)
                    if "::" not in cname:
                        cname = f"{project_name}::{cname}"
                    rclassnames.add(cname)
                props = {}
                if "properties" in rel:
                    props = {(k, v) for k, v in rel["properties"].items()}
                    props = repr(props).replace("(", "{").replace(")", "}").replace("'", '"')
                    props = props.replace("True", "true")
                    props = props.replace("False", "false")
                rinverse = False
                if "inverse" in rel:
                    rinverse = bool(rel["inverse"])
                if lang == Language.CPP:
                    rinverse = repr(rinverse).lower()
                # NOTE: Currently the constraint and delete policy are ignored!!! Maybe in the future we have a relation definition template and can reuse this.
                constraint = "MANY2MANY"
                if "cardinality" in rel:
                    constraint = rel["cardinality"].upper()
                #else:
                #    print(f"WARNING: Did not find 'cardinality' constraint in {project_name}::{classname}::{attrname}. Will use {constraint}", file=sys.stderr)
                delete_policy = "DELETENONE"
                if "delete_policy" in rel:
                    delete_policy = rel["delete_policy"].upper()
                #else:
                #    print(f"WARNING: Did not find 'delete_policy' policy in {project_name}::{classname}::{attrname}. Will use {delete_policy}", file=sys.stderr)
                subtype_of = "NONE"
                if "subtype_of" in rel:
                    subtype_of = rel["subtype_of"].upper()
                advanced_setter = False
                if "advanced_setter" in rel:
                    advanced_setter = bool(rel["advanced_setter"])
                relations[attrname] = (rname, sorted(rclassnames), constraint, delete_policy, props, subtype_of, rinverse, advanced_setter)
            except:
                raise RuntimeError(f"Error in relation definition of {classname}::{attrname}: {rel}")
    custom_uri = None
    if "uuid" in yaml_data:
        # This output is no longer possible as the output will be further used by cmake
        # print(f"WARNING: 'uuid' field in XType '{classname}' is deprecated. Consider renaming it to 'uri'");
        yaml_data["uri"] = yaml_data["uuid"]
    if "uri" in yaml_data:
        scheme = "unknown"
        if "scheme" in yaml_data["uri"]:
            scheme = yaml_data["uri"]["scheme"]
        root_path = "/"
        if "root_path" in yaml_data["uri"]:
            root_path = yaml_data["uri"]["root_path"]
        dependencies = []
        if "from" in yaml_data["uri"]:
            for attr in yaml_data["uri"]["from"]:
                attrname = attr["name"]
                required = True
                if "required" in attr:
                    required = attr["required"]
                if attrname in relations:
                    dependencies.append({"relation": attrname, "required": required})
                elif attrname in properties:
                    dependencies.append({"property": attrname, "required": required})
                elif inherit is None:
                    raise KeyError(f"Could neither find {attrname} in properties nor relations in {classname}")
        custom_uri = (scheme, root_path, dependencies)
    methods = []
    if "methods" in yaml_data:
        for method_name, method_definition in yaml_data["methods"].items():
            overrides = [method_definition]
            if "overrides" in method_definition:
                overrides = method_definition["overrides"]
            for method in overrides:
                description = ""
                if "description" in method:
                    description = str(method["description"])
                return_type = None
                method_is_template = False
                return_type_is_template = False
                template_types = []
                if "returns" in method:
                    return_type_in = method["returns"]["type"]
                    return_type, rtempl = parse_type(return_type_in, lang)
                    if rtempl is not None:
                        template_types += rtempl
                    returned_xtype = get_xtype(return_type_in)
                    # Register xtype class
                    if returned_xtype is not None:
                        classes.add(returned_xtype)
                    return_type_is_template = is_template(return_type_in)
                arguments_in = {}
                if "arguments" in method:
                    arguments_in = method["arguments"]
                arguments = []
                template_args = []
                if len(arguments_in) > 0:
                    for arg in arguments_in:
                        try:
                            arg_name = arg["name"]
                            if arg_name in [a[0] for a in arguments]:
                                raise ValueError(f"Argument {arg_name} of method {method_name} already defined in {classname}")
                            arg_type_in = arg["type"]
                            arg_xtype = get_xtype(arg_type_in)
                            arg_is_template = is_template(arg_type_in)
                            arg_type, atempl = parse_type(arg_type_in, lang)
                            if arg_type.startswith("XType"):
                                arg_type = project_name+"::"+arg_type
                            if atempl is not None:
                                template_types += atempl
                            # Register xtype class
                            if arg_xtype is not None:
                                classes.add(arg_xtype)
                            # Handle template arg
                            if arg_is_template:
                                template_args.append(arg_name)
                            arg_default = None
                            if "default" in arg:
                                arg_default = parse_default(arg["default"], arg_type_in, lang, function_argument=True)
                            arguments += [(arg_name, [arg_type, arg_default, arg_is_template, arg_xtype is not None, arg_xtype, is_xtype(arg_type_in)])]
                        except:
                            raise RuntimeError(f"Error in argument definition of {classname}::{method_name}: {arg}")
                # Check if method is a template method
                if len(template_args) > 0 or return_type_is_template:
                    method_is_template = True
                # For C++: If the return type is missing we have to set it to void
                if return_type is None:
                    return_type = "void"
                # Check whether there are xtypes in template_types we have to include
                _temp = []
                for t in template_types:
                    xt = get_xtype(t)
                    _t, _ = parse_type(t, lang)
                    _temp += [_t]
                    if xt is not None:
                        classes.add(xt)
                template_types = _temp
                methods += [(method_name, (
                    description, method_is_template, template_args, arguments, return_type_is_template, return_type,
                    "static" in method and method["static"], template_types, "const" in method and method["const"], len(overrides)>1))]
    return classname, properties, relations, sorted(classes), custom_uri, methods, default_template_types, inherit


jinja_env = Environment(
    loader=PackageLoader("xtypes_generator", "data"),
    autoescape=select_autoescape()
)


def write(file, content):
    """
    Writes the generated file content only when there are relevant changes.
    Unchanged files persist so that the compiler nows they need no rebuild.
    :param file: The filepath where to write
    :param content: The content string
    :return: None
    """
    old_content = None
    if os.path.isfile(file):
        old_content = open(file, "r").read()
    if old_content is None or content.split("\n")[3:] != old_content.split("\n")[3:]:
        os.makedirs(os.path.dirname(file), exist_ok=True)
        with open(file, "w") as f:
            f.write(content)


def generate_file(project_name, input_file, output_dir, languages, skeleton_files=None, overwrite=False):
    """
    Takes the XType template yaml file and generates the C++ files and corresponding python bindings
    :param input_file: yaml Template file for an XType
    :param output_dir: the directory where to write the c++ an python files. include/ src/ and pybind/ directories will
      be created there
    :param languages: which languages to export
    :param skeleton_files: when you want that skeleton files, the files that the user can later on customized, are
      created. specify the directory where to put them, otherwise None (default)
    :param overwrite: if set to true the file in the directory specified in skeleton_files will be overwritten
    :return: The classname of the generated XType representation and from which class this XType inherits
    """
    yaml_data = load_yaml_file(input_file)
    create_dir(os.path.join(output_dir))
    # For python bindings we also need C++ info
    # Parse the yaml and fill in the tokens to be used
    info = parse_yaml(yaml_data, project_name, Language.CPP)
    for language in languages:
        generator_comment = "Auto-generated with xtypes_generator types_generator " + datetime.datetime.now().strftime(
            "%m/%d/%Y %H:%M:%S")
        # Use jinja2 to render the template(s) and write them to file(s)
        if language == Language.CPP:
            if not input_file.endswith("xtype.yaml"):  # The xtype yaml is only used for python bindings
                create_dir(os.path.join(output_dir, "include"))
                create_dir(os.path.join(output_dir, "src"))
                base_class_header_template = jinja_env.get_template("base_class.hpp.in")
                base_class_source_template = jinja_env.get_template("base_class.cpp.in")
                skeleton_header_template = jinja_env.get_template("skeleton.hpp.in")
                skeleton_source_template = jinja_env.get_template("skeleton.cpp.in")
                base_class_header = base_class_header_template.render(project_name=project_name,
                                                                      generator_comment=generator_comment,
                                                                      classname=info[0], classes=info[3],
                                                                      custom_uri=info[4], methods=info[5],
                                                                      properties=info[1], relations=info[2],
                                                                      inherit=info[7])
                base_class_source = base_class_source_template.render(project_name=project_name,
                                                                      generator_comment=generator_comment,
                                                                      classname=info[0], classes=info[3],
                                                                      custom_uri=info[4], methods=info[5],
                                                                      properties=info[1], relations=info[2],
                                                                      inherit=info[7])
                skeleton_header = skeleton_header_template.render(project_name=project_name,
                                                                  generator_comment=generator_comment,
                                                                  classname=info[0], classes=info[3],
                                                                  custom_uri=info[4], methods=info[5],
                                                                  properties=info[1], relations=info[2],
                                                                  inherit=info[7])
                skeleton_source = skeleton_source_template.render(project_name=project_name,
                                                                  generator_comment=generator_comment,
                                                                  classname=info[0], classes=info[3],
                                                                  custom_uri=info[4], methods=info[5],
                                                                  properties=info[1], relations=info[2],
                                                                  inherit=info[7])
                write(os.path.join(output_dir, "include", "_" + info[0] + '.hpp'), base_class_header)
                write(os.path.join(output_dir, "src", "_" + info[0] + '.cpp'), base_class_source)
                if skeleton_files is not None:
                    create_dir(os.path.join(skeleton_files, "include"))
                    create_dir(os.path.join(skeleton_files, "src"))
                    inc_file = os.path.join(skeleton_files, "include", yaml_data["name"] + '.hpp')
                    src_file = os.path.join(skeleton_files, "src", yaml_data["name"] + '.cpp')
                    if not os.path.exists(inc_file) or overwrite:
                        write(inc_file, skeleton_header)
                    if not os.path.exists(src_file) or overwrite:
                        write(src_file, skeleton_source)
        elif language == Language.PYTHON:
            create_dir(os.path.join(output_dir, "pybind"))
            pybind_class_template = jinja_env.get_template("pybind_class.cpp.in")
            pybind_class_source = pybind_class_template.render(project_name=project_name,
                                                               generator_comment=generator_comment,
                                                               classname=info[0], classes=info[3],
                                                               custom_uri=info[4], methods=info[5],
                                                               properties=info[1], relations=info[2],
                                                               default_template_types=info[6],
                                                               inherit=info[7])

            write(os.path.join(output_dir, "pybind", 'py'+info[0] + '.cpp'), pybind_class_source)
        else:
            raise NotImplementedError(f"Language {language.name} not supported")
    return info[0], info[7], info[3]


def main(args):
    import argparse

    parser = argparse.ArgumentParser(
        description='The XType Generator generating programming language specific XType entities out of YAML '
                    'specifications')

    parser.add_argument('--project_name', help="The project name", type=str, required=True)
    parser.add_argument('--language', help="Desired language of the generated code",
                        choices=[x.name for x in Language] + ['ALL'], nargs="+", default="ALL")
    # TODO: Split this option into '--input' and '--inputdir'
    parser.add_argument('--input', help="The yaml template input, either a file or a directory", type=str,
                        required=True)
    # TODO: Split this option into '--outputdir_headers' and '--outputdir_sources' or '--outputdir'
    parser.add_argument('--output', help="The output directory for the auto generated files. "
                                         "For cpp include/ and src/ directories will be created in the given directory",
                        type=str, default="build/autogenerated_files")
    parser.add_argument('--skeleton_dir', help="The output directory where to put the skeleton that the user can edit."
                                               "Only for cpp. A include/ and src/ directories will be created in the "
                                               "given directory",
                        type=str, default=None)
    parser.add_argument('-f', '--overwrite_skeletons', help="Whether skeleton file shall be overwritten in each run.",
                        default=False, action="store_true")
    parser.add_argument('-p', '--do_not_create_project_registry', help="Suppress binding a project registry to a python module",
                        default=False, action="store_true")
    args = parser.parse_args(args)

    # Programming languages to convert to
    languages = [x.upper() for x in args.language] if type(args.language) == list else [args.language.upper()]
    if "ALL" in languages:
        languages = [x.name for x in Language]
    languages = [Language[x] for x in languages]

    # Generate code
    classes = []
    if os.path.isdir(args.input):
        for file in os.listdir(args.input):
            filename = os.path.join(args.input, file)
            classes += [generate_file(project_name=args.project_name, input_file=filename, output_dir=args.output, languages=languages,
                                      skeleton_files=args.skeleton_dir, overwrite=args.overwrite_skeletons)]
    else:
        classes += [generate_file(project_name=args.project_name, input_file=args.input, output_dir=args.output, languages=languages,
                                  skeleton_files=args.skeleton_dir, overwrite=args.overwrite_skeletons)]

    # print out all package dependencies for usage in cmake
    all_deps = set()
    for deps in [c[2] for c in classes]:
        for dep in deps:
            if "::" in dep:
                all_deps.add(dep[:dep.find("::")])
    print(";".join(list(all_deps)), end="")  # this is the ouput of this command

    # get all classes that inherit from another class to put them into the right order
    class_deps = {
        # child: parent
        c[0]: c[1]
        for c in classes if c[1] is not None
    }
    classes = sorted(classes, key=lambda x: x[0])
    _classnames_ordered = (["XType"] if "XType" in [c[0] for c in classes] else []) + [c[0] for c in classes if c[1] is None and c[0] != "XType"]  # put all parent on not inherited classes
    # now put all classes from class_deps as soon there parent is defined
    while len(classes) > len(_classnames_ordered):
        for c in classes:
            if c[0] not in _classnames_ordered and class_deps[c[0]] in _classnames_ordered:
                _classnames_ordered.append(c[0])

    classes = sorted(classes, key=lambda x: _classnames_ordered.index(x[0]))

    if Language.PYTHON in languages:
        pybind_module_template = jinja_env.get_template("pybind_module.cpp.in")
        generator_comment = "Auto-generated with xtypes_generator.py " + datetime.datetime.now().strftime(
            "%m/%d/%Y %H:%M:%S")
        classnames = ["XType"] + [c[0] for c in classes]
        pybind_module_source = pybind_module_template.render(project_name=args.project_name, generator_comment=generator_comment, classnames=classnames, create_project_registry=not args.do_not_create_project_registry)
        write(os.path.join(args.output, "pybind", 'pybind11_module.cpp'), pybind_module_source)
    if Language.CPP in languages and not args.do_not_create_project_registry:
        package_header_template = jinja_env.get_template("xtypes.hpp.in")
        generator_comment = "Auto-generated with xtypes_generator.py " + datetime.datetime.now().strftime(
            "%m/%d/%Y %H:%M:%S")
        classnames = (["XType"] if "XType" in classes else []) + [c[0] for c in classes]
        package_header = package_header_template.render(project_name=args.project_name, generator_comment=generator_comment, classnames=classnames)
        write(os.path.join(args.output, "include", 'xtypes.hpp'), package_header)


if __name__ == "__main__":
    import sys
    main(sys.argv)
