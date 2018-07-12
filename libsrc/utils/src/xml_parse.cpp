#include "xml_parser.h"
#include <unistd.h>
#include <boost/lexical_cast.hpp>

XmlParser::XmlParser()
{
  while (!m_stk_select_object.empty()) {
    m_stk_select_object.pop();
  }
  m_selected_object = NULL;

  init();
}

XmlParser::~XmlParser()
{
  free_mem();
}

void XmlParser::init()
{
  m_doc_ptr = NULL;
  m_elem_ptr = NULL;
  m_attr_map = NULL;
}

void XmlParser::free_mem()
{
  while (m_stk_select_object.size()) {
    m_selected_object = m_stk_select_object.top();
    m_stk_select_object.pop();
    if (m_selected_object != NULL) {
      xmlXPathFreeObject(m_selected_object);
      m_selected_object = NULL;
    }    
  }

  if (m_doc_ptr) {
    xmlFreeDoc(m_doc_ptr);
  }
}

bool XmlParser::load_file(const std::string& path)
{
  bool ret = false;

  if (access(path.c_str(), 0) != -1) {
    m_doc_ptr = xmlReadFile(path.c_str(), 0, XML_PARSE_NOBLANKS);
    if (m_doc_ptr) {
      m_elem_ptr = xmlDocGetRootElement(m_doc_ptr);
      m_attr_map = m_elem_ptr->properties;
      
      ret = true;
    }
  }
  
  return ret;
}

bool XmlParser::load_xml(const std::string& text)
{
  if (text.empty())
    return false;
  
  bool ret = false;

  m_doc_ptr = xmlParseMemory(text.c_str(), (int)text.length());
  if (m_doc_ptr) {
    m_elem_ptr = xmlDocGetRootElement(m_doc_ptr);
    m_attr_map = m_elem_ptr->properties;

    ret = true;
  }

  return ret;
}

bool XmlParser::get_xml(std::string& xml)
{
  if (!m_doc_ptr)
    return false;
  
  xmlChar* mem = NULL;
  int mem_size = 0;
  xmlDocDumpFormatMemory(m_doc_ptr, &mem, &mem_size, 0);
  xml.assign((char*)mem);
  xmlFree(mem);

  return true;
}

int XmlParser::get_attribute_count()
{
  int num = 0;
  xmlAttrPtr tmpAttr = m_attr_map;

  while (tmpAttr != NULL) {
    num++;
    tmpAttr = tmpAttr->next;
  }

  return num;
}

bool XmlParser::get_attribute(const std::string& name, std::string& value)
{
  if (!m_elem_ptr || !m_attr_map) {
    return false;
  }

  xmlChar* val;
  bool ret = false;

  value.clear();
  val = xmlGetProp(m_attr_map->parent, (xmlChar*)name.c_str());
  if (val) {
    value.assign((char*)val);
    ret = true;
  }
  xmlFree(val);

  return ret;
}

bool XmlParser::to_root()
{
  if (!m_elem_ptr)
    return false;

  m_elem_ptr = xmlDocGetRootElement(m_doc_ptr);
  m_attr_map = m_elem_ptr->properties;

  return true;
}

bool XmlParser::to_parent()
{
  if (!m_elem_ptr || is_root())
    return false;

  bool ret = false;
  xmlNodePtr tmp = m_elem_ptr->parent;
  if (tmp) {
    m_elem_ptr = tmp;
    m_attr_map = m_elem_ptr->properties;
    ret = true;
  }

  return ret;
}

bool XmlParser::to_first_child()
{
  if (!m_elem_ptr){
    return false;
  }

  bool ret = false;
  xmlNodePtr tmp = m_elem_ptr->children;

  while (tmp && xml_node_is_blank_text(tmp)) {
    tmp = tmp->next;
  }
  if (tmp) {
    m_elem_ptr = tmp;
    m_attr_map = m_elem_ptr->properties;

    ret = true;
  }

  return ret;
}

bool XmlParser::to_next_sibling()
{
  if (!m_elem_ptr) {
    return false;
  }

  bool ret = false;
  xmlNodePtr tmp = m_elem_ptr->next;
  while (tmp && xml_node_is_blank_text(tmp)) {
    tmp = tmp->next;
  }
  if (tmp) {
    m_elem_ptr = tmp;
    m_attr_map = m_elem_ptr->properties;

    ret = true;
  }

  return ret;
}

bool XmlParser::to_prev_sibling()
{
  if (!m_elem_ptr) {
    return false;
  }

  bool ret = false;
  xmlNodePtr tmp = m_elem_ptr->prev;
  while (tmp && xml_node_is_blank_text(tmp)) {
    tmp = tmp->prev;
  }
  if (tmp) {
    m_elem_ptr = tmp;
    m_attr_map = m_elem_ptr->properties;

    ret = true;
  }

  return ret;
}

bool XmlParser::to_child(int index)
{
  if (!m_elem_ptr || index < 0) {
    return false;
  }

  bool ret = false;
  xmlNodePtr tmp = m_elem_ptr->children;
  while (tmp && index > 0) {
    if (!xml_node_is_blank_text(tmp)) {
      index--;
      if (index < 0)
        break;
    }

    tmp = tmp->next;
  }

  if (tmp) {
    m_elem_ptr = tmp;
    m_attr_map = m_elem_ptr->properties;
    
    ret = true;
  }

  return ret;
}

int XmlParser::get_children_count()
{
  if (!m_elem_ptr) {
    return false;
  }

  int num = 0;
  xmlNodePtr tmp = m_elem_ptr->children;
  while (tmp) {
    if (!xml_node_is_blank_text(tmp)) {
      num++;
    }

    tmp = tmp->next;
  }

  return num;
}

xmlNodePtr XmlParser::get_node_by_xpath(const std::string& xpath,
                                        xmlNodePtr node_ptr)
{
  if(xpath.empty())
    return NULL;

  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;
  xmlNodePtr tmpNode = NULL;
    
  context = xmlXPathNewContext(m_doc_ptr);
  int ns_num = (int)m_ns_nodes.size();
  for (int i = 0; i < ns_num; i++) {
    xml_ns_node_t node = m_ns_nodes[i];
    if (xmlXPathRegisterNs(context,
                           (const xmlChar*)node.prefix.c_str(),
                           (const xmlChar*)node.uri.c_str()))
    { 
      return NULL;
    }
  }

  context->node = m_elem_ptr;
  result = xmlXPathEvalExpression((xmlChar*)xpath.c_str(), context);
        
  if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
    tmpNode = result->nodesetval->nodeTab[0]; 
  }

  xmlXPathFreeObject(result);
  context->node = NULL;
  xmlXPathFreeContext(context);
    
  return tmpNode;
}

bool XmlParser::to_first_xpath(const std::string& xpath)
{
  if (!m_elem_ptr) {
    return false;
  }

  bool ret = false;
  xmlNodePtr tmpnode = NULL;
  tmpnode = get_node_by_xpath(xpath, m_elem_ptr);
  if (tmpnode) {
    m_elem_ptr = tmpnode;
    m_attr_map = m_elem_ptr->properties;

    ret = true;
  }

  return ret;
}

void XmlParser::get_node_name(std::string& name)
{
  name.clear();
  if (m_elem_ptr) {
    name.assign((char*)m_elem_ptr->name);
  }
}

void XmlParser::get_node_text(std::string& text)
{
  text.clear();
  if(!m_elem_ptr) {
    return;
  }
  
  xmlNodePtr tmp_child;
  tmp_child = m_elem_ptr->children;
  while (tmp_child != NULL) {
    xmlElementType dtype = tmp_child->type;
    if (dtype == XML_TEXT_NODE || dtype == XML_CDATA_SECTION_NODE) {
      xmlChar* value = xmlNodeGetContent(m_elem_ptr);
      text = std::string((char*)value);


      xmlFree(value);
      break;
    }
    tmp_child= tmp_child->next;
  }
}

void XmlParser::get_node_xml(std::string& xml)
{
  xmlBufferPtr buffer_ptr = xmlBufferCreate();

  if (-1 != xmlNodeDump(buffer_ptr, m_doc_ptr, m_elem_ptr, 1, 0)) {
    xml.assign((char*)buffer_ptr->content);
  }
  
  if (buffer_ptr != NULL)
    xmlBufferFree(buffer_ptr);
}

bool XmlParser::is_root()
{       
  if (m_elem_ptr == NULL) {
    return false;
  }

  bool ret = false;
  xmlNodePtr tmpNode = xmlDocGetRootElement(m_doc_ptr);
  if (m_elem_ptr == tmpNode)
  {
    ret = true;
    //xmlFreeNode(tmpNode);
  }
  
  return ret;
}

bool XmlParser::is_empty()
{
  return (m_elem_ptr == NULL);
}

int XmlParser::select_nodes(const std::string& xpath)
{
  if(!m_elem_ptr) {
    return 0;
  }

  int num = 0;
  xmlXPathContextPtr context;
  context = xmlXPathNewContext(m_doc_ptr);
  int ns_num = (int)m_ns_nodes.size();
  for (int i = 0; i < ns_num; i++) {
    xml_ns_node_t node = m_ns_nodes[i];
    if (xmlXPathRegisterNs(context,
                           (const xmlChar*)node.prefix.c_str(),
                           (const xmlChar*)node.uri.c_str()))
    {
      return 0;
    }
  }

  m_selected_object = xmlXPathEvalExpression((xmlChar*)xpath.c_str(), context);
  m_stk_select_object.push(m_selected_object);
  if (xmlXPathNodeSetIsEmpty(m_selected_object->nodesetval)) {
    m_selected_list = NULL;
  }
  else {
    m_selected_list = m_selected_object->nodesetval;
    m_list_num = -1;
    num = m_selected_list->nodeNr;
  }

  xmlXPathFreeContext(context);

  return num;
}

bool XmlParser::to_next_selected_node() {
  return to_selected_node(++m_list_num);
}

int XmlParser::get_selected_node_count()
{
  if(!m_elem_ptr || !m_selected_list) {
    return 0;
  }
  
  return m_selected_list->nodeNr;
}

bool XmlParser::to_selected_node(int index)
{
  if(!m_elem_ptr) {
    return false;
  }

  bool ret = false;
  if (m_selected_list) {
    if ((index >= 0) && (index < get_selected_node_count())) {
      m_elem_ptr = m_selected_list->nodeTab[index];
      if (m_elem_ptr==NULL) {
        return ret;
      }
      
      m_attr_map = m_elem_ptr->properties;
      m_list_num = index;
      ret = true;
    }
  }

  return ret;
}

static inline int process_xpath(const std::string& complete_path, std::string& xpath, std::string& attr_name)
{
  xpath.clear();
  attr_name.clear();
  size_t pos = complete_path.find("@");
  if (std::string::npos == pos) {
    xpath = complete_path;
    return 0;
  }
  
  xpath = complete_path.substr(0, pos - 1);
  attr_name = complete_path.substr(pos + 1, complete_path.size() - pos - 1);
  return 1;
}

int XmlParser::push()
{
  if (!m_elem_ptr) {
    return -1;
  }
  m_stk_element.push(m_elem_ptr);
  m_stk_attr_map.push(m_attr_map);
  m_stk_node_list.push(m_selected_list);
  return (int)m_stk_element.size();
}

bool XmlParser::pop()
{
  bool ret = false;
  if (m_stk_element.size() > 0) {
    m_elem_ptr = m_stk_element.top();
    m_stk_element.pop();
    m_attr_map = m_stk_attr_map.top();
    m_stk_attr_map.pop();
    m_selected_list = m_stk_node_list.top();
    m_stk_node_list.pop();
    ret = true;
  }

  return ret;
}

bool XmlParser::get_value(const std::string& complete_xpath,
                          std::string& value)
{
  bool ret = false;
  std::string attr_name;
  std::string xpath;
  xmlNodePtr tmp;
  int ret_tmp;
  
  value.clear();
  if(!m_elem_ptr) {
    goto LabelReturn;
  }

  ret_tmp = process_xpath(complete_xpath, xpath, attr_name);
  try {
    tmp = get_node_by_xpath(xpath, m_elem_ptr);
    if (tmp != NULL) {
      xmlElementType ntype;
      ntype = tmp->type;
      switch (ntype) {
      case XML_ELEMENT_NODE :
        push();
        m_elem_ptr = tmp;
        m_attr_map = tmp->properties;
        if (ret_tmp == 0) {
          get_node_text(value);
        }
        else {
          get_attribute(attr_name, value);
        }
        pop();
        break;
        
      case XML_ATTRIBUTE_NODE :
        break;
        
      default:
        break;
      }
      ret = true;
    }
    else {
      // error log here
    }
  }
  catch(...) {
    // error log here
  }

 LabelReturn:
  return ret;
}

bool XmlParser::get_value(const std::string& xpath,
                          int& value)
{
  bool ret = false;
  std::string ret_value;

  ret = get_value(xpath, ret_value);
  if (ret) {
    if (ret_value.empty())
      value = 0;
    else
      value = atoi(ret_value.c_str());
  }

  return ret;
}

bool XmlParser::get_value(const std::string& xpath,
                          unsigned long long& value)
{
  bool ret = false;
  std::string ret_value;

  ret = get_value(xpath, ret_value);
  if (ret) {
    ret = boost::lexical_cast<unsigned long long>(ret_value);
    //value = atoi(ret_value.c_str());
  }  

  return ret;
}

bool XmlParser::xml_node_is_blank_text(xmlNodePtr node)
{
  bool ret = false;
  
  if (XML_TEXT_NODE == node->type
      && node->content
      && '\n' == ((char*)node->content)[0]) {
    ret = true;
  }

  return ret;
}

bool XmlParser::save(const std::string& path)
{  
  bool ret = false;
  char *psz=NULL;
  if (xmlSaveFormatFile(path.c_str(), m_doc_ptr, 1) != -1) {
    ret = true;
  }

  return ret;
}

bool XmlParser::set_attribute(const std::string& attr_name,
                              const std::string& attr_value)
{
  bool ret = false;
  char *psz=NULL;
  char *pszval=NULL;
  if (!m_elem_ptr)
  {
    return ret;
  }
  
  if (xmlSetProp(m_elem_ptr,
                 (const xmlChar*)attr_name.c_str(),
                 (const xmlChar*)attr_value.c_str())) {
    m_attr_map = m_elem_ptr->properties;
    ret = true;
  }

  return ret;
}

bool XmlParser::remove_attribute(const std::string& attr_name)
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }
  if (-1 != xmlUnsetProp(m_elem_ptr, (xmlChar*)attr_name.c_str())) {
    m_attr_map = m_elem_ptr->properties;
    ret = true;
  }

  return ret;
}

bool XmlParser::clear_attributes()
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }
  xmlAttrPtr attr_tmp = m_elem_ptr->properties;
  while (attr_tmp != NULL) {
    xmlAttrPtr attr_next = attr_tmp->next;
    if (-1 == xmlUnsetProp(m_elem_ptr, attr_tmp->name)) {
      return ret;
    }
    attr_tmp = attr_next;
  }
  ret = true;

  return ret;
}

bool XmlParser::create_document(const std::string& docname)
{
  bool ret = true;
  
  if (m_doc_ptr != NULL) {
    xmlFreeDoc(m_doc_ptr);
  }
  
  m_doc_ptr = xmlNewDoc((const xmlChar*)"1.0");
  m_elem_ptr = NULL;
  m_attr_map = NULL;
  if (!docname.empty()) {
    ret = save(docname);
  }

  return ret;
}

bool XmlParser::create_document_element(const std::string& name)
{
  bool ret = false;
  xmlNodePtr temp_node;
        
  if (m_doc_ptr == NULL) {
    create_document("");
  }
  //  if (m_elem_ptr != NULL) {
  //    return ret;
  //  }

  std::string strname = name;
  while (strname.find("/") == 0) {
    strname = strname.erase(0, 1);
    size_t pos = strname.find("/");
    if (pos != std::string::npos)
      strname.erase(pos);
  }

  if ((temp_node = xmlNewNode(NULL, (xmlChar*)strname.c_str()))) {
    xmlDocSetRootElement(m_doc_ptr,  temp_node);
    m_elem_ptr = temp_node;
    ret = true;
  }

  return ret;
}

void XmlParser::open_or_create_path(const std::string& path)
{
  size_t pos = 0;
  std::string tmppath;
  
  to_root();

  // Create path when more than one layer
  while ((pos = path.find("/", ++pos)) != std::string::npos) {
    tmppath = path.substr(0, pos);
    if (!to_first_xpath(tmppath)) {
      if ( is_empty() ) {
        if (tmppath[0] == '/') {
          tmppath.erase(0, 1);
        }
        create_document_element(tmppath);
      }
      else {
        int count;
        append_child(tmppath.substr(tmppath.find_last_of('/') + 1), count, true);
      }
    }
  }

  // Create path when only one layer or the last layer
  tmppath = path;
  if (!to_first_xpath(tmppath)) {
    if (is_empty())     { // only one layer
      if (tmppath[0] == '/') {
        tmppath.erase(0, 1);
      }
      create_document_element(tmppath);
    }
    else {      // create last layer
      int count;
      append_child(tmppath.substr(tmppath.find_last_of('/') + 1), count, true);
    }
  }
}

bool XmlParser::append_child(const std::string& nodename,
                             int& newcount,
                             bool enternode /*= false*/)
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }
  
  xmlNodePtr tmp_node= xmlNewNode(NULL, (xmlChar*)nodename.c_str());
  xmlNodePtr content = xmlNewText((xmlChar*)"");
  if (tmp_node != NULL) {
    xmlAddChild(m_elem_ptr, tmp_node);
    xmlAddChild(tmp_node, content);

    newcount = get_children_count();
    if (enternode) {
      m_elem_ptr = tmp_node;
      m_attr_map = m_elem_ptr->properties;
    }
    ret = true;
  }

  return ret;
}

bool XmlParser::set_node_name(const std::string& name)
{
  bool ret = false;
  if (m_elem_ptr == NULL) {
    return ret;
  }


  xmlNodeSetName(m_elem_ptr, (xmlChar*)name.c_str());
  ret = true;

  return ret;
}

bool XmlParser::set_node_text(const std::string& text)
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }

  //  ProcessSpecialChars(nodetext, saValue);
  xmlNodeSetContent(m_elem_ptr,  (xmlChar*)text.c_str());
  ret = true;

  return ret;
}

bool XmlParser::insert_before(const std::string& nodename,
                              int index,
                              int& newcount,
                              bool enternode)
{
  bool ret = false;
  
  if (!m_elem_ptr) {
    return ret;
  }

  xmlNodePtr tmp_node = xmlNewDocNode(m_doc_ptr, NULL, (xmlChar*)nodename.c_str(), NULL);

  if (tmp_node != NULL) {
    xmlNodePtr node_before = get_xml_child_by_index(m_elem_ptr, index);
    if (node_before!=NULL) {
      if (xmlAddPrevSibling(node_before, tmp_node) != NULL) {
        xmlNodePtr content = xmlNewText((xmlChar*)"");
        xmlAddChild(tmp_node,content);

        newcount = get_children_count();
        if (enternode) {
          m_elem_ptr = tmp_node;
          m_attr_map = m_elem_ptr->properties;
        }
        ret = true;
      }
    }
  }

  return ret;
}

xmlNodePtr XmlParser::get_xml_child_by_index(xmlNodePtr parent, int index)
{
  xmlNodePtr tmp_node = parent->children;
  while (tmp_node != NULL && index > 1) {
    if (!xml_node_is_blank_text(tmp_node)) {
      index--;
    }
    tmp_node = tmp_node->next;
  }

  return tmp_node;
}

void XmlParser::reset_selected()
{
  if (m_selected_list != NULL) {
    m_list_num = 0;
  }
}

bool XmlParser::remove_current()
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }

  if (!is_root()) {
    xmlNodePtr parent = m_elem_ptr->parent;
    //use NULL to replace means delete
    xmlNodePtr del_node = xmlReplaceNode(m_elem_ptr, NULL);
    xmlFreeNode(del_node);
    m_elem_ptr = parent;
    m_attr_map = m_elem_ptr->properties;
    ret = true;
  }

  return ret;
}

bool XmlParser::clear_children()
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }
  
  xmlNodePtr tmp_node = m_elem_ptr->children;
  while (tmp_node!=NULL) {
    xmlNodePtr del_node = tmp_node;
    tmp_node = tmp_node->next;
    del_node = xmlReplaceNode(del_node, NULL);
    xmlFreeNode(del_node);
  }

  ret = true;
  return ret;
}

bool XmlParser::set_value(const std::string& xpath,
                          const std::string& value)
{
  bool ret = false;
  if (!m_elem_ptr) {
    return ret;
  }
  
  xmlNodePtr tmp, tmpcur;
  bool created = false;
  while (true) {
    tmp = get_node_by_xpath(xpath, m_elem_ptr);
    if (tmp==NULL && !created) {
      open_or_create_path(xpath);
      created = true;
    }
    else
      break;
  }

  if (tmp != NULL) {
    xmlElementType ntype;
    ntype = tmp->type;
    switch(ntype) {
    case XML_ELEMENT_NODE: {
      tmpcur = m_elem_ptr;
      m_elem_ptr = tmp;
      set_node_text(value);
      m_elem_ptr = tmpcur;
      ret = true;
    }
      break;
        
    case XML_ATTRIBUTE_NODE: {
    }
      break;

    default:
      break;
    }
  }

  return ret;
}

bool XmlParser::set_value(const std::string& xpath,
                          int value)
{
  char buffer[20] = {0};
  snprintf(buffer, sizeof(buffer), "%d", value);

  return set_value(xpath, std::string(buffer));
}

void XmlParser::add_namespace(const std::string &prefix, const std::string &uri)
{
  xml_ns_node_t node;
  node.prefix = prefix;
  node.uri = uri;
  m_ns_nodes.push_back(node);
}
