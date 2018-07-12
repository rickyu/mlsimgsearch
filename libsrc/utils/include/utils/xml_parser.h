#ifndef UTILS_XML_PARSER_H_
#define UTILS_XML_PARSER_H_

#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <string>
#include <stack>
#include <vector>

typedef struct _xml_ns_node {
  std::string prefix;
  std::string uri;
}xml_ns_node_t;

class XmlParser
{
public:
  XmlParser();
  ~XmlParser();

  /**
     Load file into object.
     @return If it's normal xml file return true, otherwise, false instead.
   */
  bool load_file(const std::string& path);

  /**
     Load xml text into object.
     @return If the xml text is normal return true, otherwise, false instead.
   */
  bool load_xml(const std::string& text);

  /**
     Get current xml text of current node.
   */
  bool get_xml(std::string& text);

  /**
     Get attribute count of current node.
   */
  int get_attribute_count();

  /**
     Get specified attribute value of current node.
   */
  bool get_attribute(const std::string& name, std::string& value);

  /**
     Go to root node.
   */
  bool to_root();

  /**
     Go to parent node.
   */
  bool to_parent();

  /**
     Go to first child node.
   */
  bool to_first_child();

  /**
     Go to next child node.
   */
  bool to_next_sibling();

  /**
     Go to previous child node.
   */
  bool to_prev_sibling();

  /**
     Go to child by index.
   */
  bool to_child(int index);

  /**
     Get children count of current node.
   */
  int get_children_count();

  /**
     Go to first node selected with xpath.
   */
  bool to_first_xpath(const std::string& xpath);

  /**
     Go to the first node with the pointed name.
   */
  bool to_first_child(const std::string& name);

  /**
     Get the name of current node.
   */
  void get_node_name(std::string& name);

  /**
     Get the text of current node if the node has text.
   */
  void get_node_text(std::string& text);

  /**
     Get xml string of current node.
   */
  void get_node_xml(std::string& xml);

  /**
     Check current node is root node.
   */
  bool is_root();

  /**
     Check current node is empty.
   */
  bool is_empty();


  /**
     Select some nodes with xpath.
     @return number of nodes selected.
   */
  int select_nodes(const std::string& xpath);

  /**
     Get selected nodes count.
   */
  int get_selected_node_count();

  bool to_next_selected_node();

  /**
     Go to selected node by index.
   */
  bool to_selected_node(int index);
  
  /**
     Get the value of xpath.
   */
  bool get_value(const std::string& complete_xpath, std::string& value);
  bool get_value(const std::string& complete_xpath, int& value);
  bool get_value(const std::string& complete_xpath, unsigned long long& value);

  /** 
   * save xml to a file.
   */
  bool save(const std::string& path);

  /**
     add or update the attribute value of current node,
     if the attribte is exists, it's a updating operation.
     if the attribute is not exists, it's a inserting operation.
   */
  bool set_attribute(const std::string& attr_name, const std::string& attr_value);

  /**
     remove a attribute of current node.
   */
  bool remove_attribute(const std::string& attr_name);

  /**
     clear all attributes of current node.
   */
  bool clear_attributes();

  /**
     update current node name.
   */
  bool set_node_name(const std::string& name);

  /**
     update text of current node.
   */
  bool set_node_text(const std::string& text);

  bool insert_before(const std::string& nodename,
                     int index,
                     int& newcount,
                     bool enternode);

  /**
     remove current node and it's children.
   */
  bool remove_current();

  /**
     clear all child nodes of current node.
   */
  bool clear_children();

  /**
     set node value of the given xpath.
   */
  bool set_value(const std::string& xpath,
                 const std::string& value);
  bool set_value(const std::string& xpath,
                 int value);

  /**
     create a empty xml file. it only contains one line: <?xml version="1.0"?>
   */
  bool create_document(const std::string& docname);

  /**
     create a xml file and append a root node.
   */
  bool create_document_element(const std::string& name);

  /**
     append a child of current node.
   */
  bool append_child(const std::string& nodename, int& newcount, bool enternode = false);

  /**
   */
  void open_or_create_path(const std::string& path);

  void add_namespace(const std::string &prefix, const std::string &uri);
  
protected:
  xmlNodePtr get_xml_child_by_index(xmlNodePtr parent, int index);
  xmlNodePtr get_node_by_xpath(const std::string& xpath,
                               xmlNodePtr node_ptr);
  bool xml_node_is_blank_text(xmlNodePtr node);
  int push();
  bool pop();
  void free_mem();
  void init();
  void reset_selected();

  
private:
  xmlDocPtr                     m_doc_ptr;
  xmlNodePtr                    m_elem_ptr;
  xmlAttrPtr                    m_attr_map;
  xmlNodeSetPtr                 m_selected_list;
  xmlXPathObjectPtr             m_selected_object;
  std::stack<xmlXPathObjectPtr> m_stk_select_object;
  std::stack<xmlNodePtr>                m_stk_element;
  std::stack<xmlAttrPtr>                m_stk_attr_map;
  std::stack<xmlNodeSetPtr>             m_stk_node_list;
  int                           m_list_num;
  std::vector<xml_ns_node_t>    m_ns_nodes;
};

#endif
