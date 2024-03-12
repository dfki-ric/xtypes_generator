# Template Usage

Let's focus on how to write templates. You'll find a sample in doc/template.yaml.

A template file has 6 fields/properties you can specify at which we will look now in detail:

- name
- inherit
- properties
- relations
- uuid
- methods

From this template file the xtype_generator will generate the basic c++ code structure which you will have to populate with your source code.
Also the generator will provide the corresponding python bindings to your code; those are ready to use, and you have nothing to do there.

## Name
This is obviously the name of the XType you are going to generate.

## Inherit
This field is optional. You can use it if you want to have your XType to be a child of another XType.

> NOTE: You must not specify `inherit: XType` this inheritance is already given by default.

## Properties
In this section you define which properties your XType will have.
So to add a property to your XType you'll have to specify the name type and a default value in the following structure.

```yaml
properties:
    a_string:
        type: STRING
        default: "\"DEFAULT\""
        allowed: ["\"A\"","\"B\"","\"C\""]  # optional
    a_small_integer:
        type: INTEGER
        default: 0
    a:
        nested:
            property:
                type: ...
```

Pleases see the [Type](#type) section and also doc/template.yml for further information on how to specify other types.

With `allowed` you can limit the set of allowed values for your property. This way you can create an enum-like property.

If you want to further restrict/customize the access to the property you can use `advanced_setter: true` to let the Generator create an override for the `set_` method.

## Relations
Here you define how your XType is related to other XTypes. Each relation type defines a cardinality constraint e.g. that one XType can have (HAS) multiple other XTypes instances (ONE2MANY), but can only be an ALIAS_OF one other XType instance (ONE2ONE).

Please see doc/Relations.md for the existing Relation types.

The relations are defined using the following structure:
```yaml
relations:
    children:  # name of the relation
        type: HAS
        other_classname: Template # the name of the other XType
        inverse: false
        properties: {}
    parent:
        type: HAS
        other_classname: Template
        inverse: true   
        properties: {}
```
The `inverse` field show us that we correspond to a relation in the other direction. In this example: HAS defines a relation with the cardinality constraint ONE2MANY: The new XType can have multiple children. With inverse we show the other direction of the relation: The new XType is a child of only one parent.

You may optionally define some properties for your relation.

If you want to restrict/customize access to the relation you can specify `advanced_setter: true` to let the generator create an override for the `add_` method. In there you can f.e. check if adding an XType to that relation is allowed or not.

## UUID
This field specifies by which properties and relations your XType can be recognized as unique.

## Methods
Here you can define member methods your new XType will have.
A definition can look like the following. If your method has no arguments or returns nothing feel free to just not specify those parameters. The descriptionis optional as well, but hey, please do your documentation. ;)
```yaml
methods:
  a_method:
      arguments:
          a_string:
              type: STRING
              default: "\"DEFAULT\""
          a_bool:
              type: BOOLEAN
              default: True
          a_xtype:
              type: XTYPE(Template)
      description: "A description of what a_method should do"
      returns:
          type: XTYPE(TemplatePtr)
```
Regarding the types (explained in the following section) please note that XTYPEs in the arguments are always passed as pointers so you define just XTYPE(Name of the XType). For the return type you have to add the Ptr suffix to your XType name if you want to return the pointer, otherwise a value will be returned.

With `static: true` a method can be declared static such that it is callable without an instance of the corresponding XType.

With `const: true` a method can be marked that it shall not modify the XType. The compiler can check this during compile time.

## Types
For your properties and methods you can make use of the following types. There are some basic atomic types:
- _STRING_
- _INTEGER_
- _INTEGER64_
- _BOOLEAN_
- _FLOAT_
- _FLOAT64_
- _JSON_
- _ANY_

Then there are some other (mostly container) types, which take arguments. Instead of the Ts you can use any other type explained here.
- _SET(T)_
- _PAIR(T1, T2)_
- _MAP(T1, T2)_
- _VECTOR(T)_
- _FUNCTION(T)_

You also can define template types, in case you want to create template methods. These types only work for methods.
To use a template simply use the _TEMPLATE_ specifier. If you want to limit the template to a set of types (which is highly recommended) add those types in a list e.g.: _TEMPLATE[STRING, INT]_
If you don't give this list, the template is generated for these types: [_STRING_, _INTEGER_, _INTEGER64_, _BOOLEAN_, _FLOAT_, _FLOAT64_, _JSON_]

Finally there is the XTYPE specifier: _XTYPE(Name of an XType specialization)_
With this specifier you state that you expect this to be an subclass of XType. Please note that for XTypes you have to specify for return types whether you want to return a pointer (_XTYPE(Component)_) or a value(_XTYPE(ComponentPtr)_).
XType arguments are always passed as pointer, so you don't need the _Ptr_ suffix.

If you have other custom types (not recommended to use them in the templates) you can just type there name, but note that the xtypes_generator won't be able to create python bindings for it, so you either have to bind those yourself, adapt the pybind files or do this only for projects where you don't generate the python bindings.
Also make sure that you include those types definitions.
