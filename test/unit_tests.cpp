#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <iostream>
// Include XTypes
#include  "XType.hpp"




using namespace xtypes;


TEST_CASE("Test XType construction and interface", "XType")
{
    INFO("Construct an XType");
    XType my_xtype;
    REQUIRE(my_xtype.get_classname() == xtypes::XType::classname);
    REQUIRE(!my_xtype.uri().empty());

    SECTION("Define and use properties")
    {
        my_xtype.define_property("a property", nl::json::value_t::string, {}, "a value");
        REQUIRE(my_xtype.has_property("a property"));
        my_xtype.define_property("an integral property", nl::json::value_t::number_integer, {}, 1);
        REQUIRE(my_xtype.has_property("an integral property"));
        my_xtype.define_property("a real property", nl::json::value_t::number_float, {}, 1.1);
        REQUIRE(my_xtype.has_property("a real property"));
        REQUIRE(my_xtype.get_property("a real property") == 1.1);
        my_xtype.set_property("a real property", 1.2);
        REQUIRE(my_xtype.get_property("a real property") == 1.2);
        my_xtype.define_property("direction", nl::json::value_t::string, {"in","out"}, "out");
        REQUIRE(my_xtype.get_allowed_property_values("direction") == std::set<nl::json>{"in", "out"});
        REQUIRE_THROWS(my_xtype.set_property("direction", "left"));

        SECTION("Test relation definition and usage")
        {
            REQUIRE(!my_xtype.has_relation("non-existing relation"));
            REQUIRE_THROWS(my_xtype.get_relation("non-existing relation"));
            REQUIRE_THROWS(my_xtype.get_relations_dir("non-existing relation"));
            my_xtype.define_relation("a relation", RelationType::CONNECTED_TO, {XType::classname}, {XType::classname});
            REQUIRE(my_xtype.has_relation("a relation"));
            REQUIRE(my_xtype.get_relations_dir("a relation") == true);
            const Relation &rel = my_xtype.get_relation("a relation");
            REQUIRE(rel.relation_type == RelationType::CONNECTED_TO);
            REQUIRE(rel.from_classnames == std::set{XType::classname});
            REQUIRE(rel.to_classnames == std::set{XType::classname});

            SECTION("Test fact definition and usage")
            {
                REQUIRE_THROWS(my_xtype.has_facts("non-existing relation"));
                REQUIRE(!my_xtype.has_facts("a relation"));
                std::shared_ptr<XType> other_xtype = std::make_shared<XType>();
                REQUIRE_THROWS(my_xtype.add_fact("non-existing relation", other_xtype));
                my_xtype.add_fact("a relation", other_xtype);
                REQUIRE(my_xtype.has_facts("a relation"));
                REQUIRE(my_xtype.get_facts("a relation").size() == 1);
                REQUIRE(my_xtype.get_facts("a relation")[0].target.lock() == other_xtype);
                // Double add_facts() on the same object should not create a new fact
                my_xtype.add_fact("a relation", other_xtype);
                REQUIRE(my_xtype.get_facts("a relation").size() == 1);
                REQUIRE_THROWS(my_xtype.remove_fact("non-existing relation", other_xtype));
                my_xtype.remove_fact("a relation", other_xtype);
                REQUIRE(my_xtype.get_facts("a relation").size() == 0);
            }
        }
    }
}

TEST_CASE("Test Fact construction and usage", "Fact")
{
    INFO("Construct a Fact");
    std::shared_ptr<XType> xtype_ptr = std::make_shared<XType>();
    nl::json props = {{"test_property1", "content1"}};
    Fact fact1(xtype_ptr, props);
    Fact fact2(xtype_ptr, nl::json{{"test_property2", "content2"}});
    std::vector<Fact> vec{fact1, fact2};

    INFO("Use the fact");
    REQUIRE(fact1.target.lock() == xtype_ptr);
    REQUIRE(fact1.edge_properties["test_property1"] == "content1");
    int i = 1;
    for (const auto &[x, p] : vec)
    {
        REQUIRE(x.lock() == xtype_ptr);
        REQUIRE(p["test_property" + std::to_string(i)] == "content" + std::to_string(i));
        i++;
    }
}
