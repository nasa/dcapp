#include "config.h"
#include "elem.h"
#include "lookup.h"
#include "../utils/file.h"

#include <libxml/parser.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void _dc_app_clean_xml_node(xmlNodePtr node, DcAppLookup *lookup, const char *directory);

DcAppConfig *dc_app_config_create(const char *config_path) {
    DcAppConfig *config = (DcAppConfig *)malloc(sizeof(DcAppConfig));

    // get current working directory
    char cwd[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_cwd(cwd, DC_UTILS_FILEPATH_BUFFER_SIZE);

    // get canonical config file path
    config->config_file_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    if (dc_utils_is_relative_path(config_path)) {
        char abs_config_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        dc_utils_join_paths(cwd, config_path, abs_config_path, sizeof(abs_config_path));
        dc_utils_canonicalize_path(abs_config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    } else {
        dc_utils_canonicalize_path(config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    }

    // get config file dir
    config->config_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_get_directory(config->config_file_path, config->config_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);

    // get exe dir
    char exe_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_exe_path(exe_path, sizeof(exe_path));
    char exe_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_directory(exe_path, exe_dir, sizeof(exe_dir));

    // get + create cache dir
    config->cache_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_join_paths(exe_dir, "cache", config->cache_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->cache_dir_path);

    // get + create log dir
    config->log_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_join_paths(exe_dir, "logs", config->log_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->log_dir_path);

    // get XML doc
    config->xml_doc = xmlReadFile(config->config_file_path, "UTF-8", XML_PARSE_NOBLANKS);
    if (!config->xml_doc) {
        fprintf(stderr, "DCAPP dc_app_config_create: unable to read config file '%s'\n", config->config_file_path);
    }
    config->xml_doc_is_cleaned = false;

    return config;
}

void dc_app_config_cleanup(DcAppConfig *config) {
    free(config->config_file_path);
    free(config->config_dir_path);
    free(config->cache_dir_path);
    free(config->log_dir_path);
    xmlFreeDoc(config->xml_doc);
}

void dc_app_config_clean_xml(DcAppConfig *config, DcAppLookup *lookup) {

    // get root element
    xmlNodePtr node = xmlDocGetRootElement(config->xml_doc);
    if (node == NULL) {
        fprintf(stderr, "DCAPP dc_app_config_clean_xml(): unable to get root element of config file\n");
    }

    // verify root node is valid
    if (dc_app_xml_node_to_elem_type(node) != DC_APP_ELEM_TYPE_DCAPP) {
        fprintf(stderr, "DCAPP dc_app_config_clean_xml(): configuration root element is not DCAPP\n");
    }

    // clean XML file
    _dc_app_clean_xml_node(node, lookup, config->config_dir_path);
    config->xml_doc_is_cleaned = true;
}

void _dc_app_clean_xml_node(xmlNodePtr node, DcAppLookup *lookup, const char *directory) {

    // remove if not an element
    if (node->type != XML_ELEMENT_NODE && node->type != XML_TEXT_NODE) {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
    } else {
        // processing before targeting children
        switch (dc_app_xml_node_to_elem_type(node)) {

            case DC_APP_ELEM_TYPE_CONSTANT: {
                char *name = (char *)xmlGetProp(node, (xmlChar *)("Name"));
                if (!name) {
                    fprintf(stderr, "DCAPP _dc_app_clean_xml_node(): 'Name' attribute missing in <Constant> definition\n");
                }
                char cleaned_name[DC_VALUE_STRING_BUFFER_SIZE];
                dc_app_lookup_dereference_constants(lookup, name, cleaned_name, sizeof(cleaned_name));
                free(name);

                char *value = (char *)xmlNodeGetContent(node);
                if (!value) {
                    fprintf(stderr, "DCAPP _dc_app_clean_xml_node(): Node content missing in <Constant> definition\n");
                }
                char cleaned_value[DC_VALUE_STRING_BUFFER_SIZE];
                dc_app_lookup_dereference_constants(lookup, value, cleaned_value, sizeof(cleaned_value));
                free(value);

                dc_app_lookup_set_const_by_name(lookup, name, value);
                break;
            }

            // remove "Dummy" level, keep children
            case DC_APP_ELEM_TYPE_DUMMY: {
                // requires some finagling if it contains children. Otherwise, unlink it normally
                if (node->children) {
                    xmlNodePtr first_child = node->children;
                    xmlNodePtr last_child  = node->last;

                    // update parent
                    if (node->parent) {
                        if (node->parent->children == node) {
                            node->parent->children = first_child;
                        }
                        if (node->parent->last == node) {
                            node->parent->last = last_child;
                        }
                    }

                    // update siblings
                    if (node->prev) {
                        node->prev->next = first_child;
                    }
                    if (node->next) {
                        node->next->prev = last_child;
                    }

                    // update children
                    for (xmlNodePtr curr_child = first_child; curr_child; curr_child = curr_child->next) {
                        curr_child->parent = node->parent;
                    }
                    first_child->prev = node->prev;
                    last_child->next  = node->next;

                    // unlink node
                    node->children = NULL;
                    node->last     = NULL;
                    node->parent   = NULL;
                    node->next     = NULL;
                    node->prev     = NULL;
                } else {
                    xmlUnlinkNode(node);
                }
                xmlFreeNode(node);
                break;
            }

            case DC_APP_ELEM_TYPE_INCLUDE: {

                // get include file name
                char *filepath = (char *)xmlNodeGetContent(node);
                if (!filepath) {
                    fprintf(stderr, "DCAPP _dc_app_clean_xml_node(): Node content missing in <Include> definition\n");
                }
                char cleaned_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
                dc_app_lookup_dereference_constants(lookup, filepath, cleaned_filepath, sizeof(cleaned_filepath));
                free(filepath);

                // get canonical path
                char canon_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
                if (dc_utils_is_relative_path(cleaned_filepath)) {
                    char abs_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
                    dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                    dc_utils_canonicalize_path(abs_filepath, canon_filepath, sizeof(canon_filepath));
                } else {
                    dc_utils_canonicalize_path(cleaned_filepath, canon_filepath, sizeof(canon_filepath));
                }

                // create new prop, assign to <Include> node
                xmlAttrPtr directory_prop = xmlNewProp(node, (xmlChar *)("Directory"), (xmlChar *)(directory));

                // read XML file
                xmlDocPtr sub_doc = xmlReadFile(canon_filepath, NULL, XML_PARSE_NOBLANKS);
                if (!sub_doc) {
                    fprintf(stderr, "DCAPP _dc_app_clean_xml_node(): Unable to read config file %s\n", canon_filepath);
                }

                // get root element
                xmlNodePtr sub_node = xmlDocGetRootElement(sub_doc);
                if (!sub_node) {
                    fprintf(stderr, "DCAPP _dc_app_clean_xml_node(): Unable to get root element of config file %s\n", canon_filepath);
                }

                // replace child (text) with subnode
                xmlNodePtr new_node = xmlDocCopyNode(sub_node, sub_node->doc, 1);
                xmlNodePtr old_node = xmlReplaceNode(node->children, new_node);

                // free old node/doc
                xmlUnlinkNode(old_node);
                xmlFreeNode(old_node);
                xmlFreeDoc(sub_doc);
                break;
            }
            default:
                break;
        }

        // clean children
        xmlNodePtr child = node->children;
        while (child) {
            xmlNodePtr child_next = child->next;
            _dc_app_clean_xml_node(child, lookup, directory);
            child = child_next;
        }
    }
}

void dc_app_config_save_to_file(DcAppConfig *config, const char *filepath) {
    xmlSaveFormatFile(filepath, config->xml_doc, 1);
}
