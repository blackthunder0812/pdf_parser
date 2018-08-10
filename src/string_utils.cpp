#include "string_utils.hpp"


nlohmann::json add_json_node(PDF_Section_Node &current_node, unsigned int &id)
{
    nlohmann::json json_pdf_section;
    current_node.main_section->id = id++;
    json_pdf_section["id"] = current_node.main_section->id;
    json_pdf_section["title"] = current_node.main_section->title;
    if (current_node.parent_node) {
        json_pdf_section["parent_id"] = current_node.parent_node->main_section->id;
    }
    for (PDF_Paragraph& paragraph : current_node.main_section->paragraphs) {
        nlohmann::json json_pdf_paragraph;
        json_pdf_paragraph["paragraph"] = paragraph.paragraph;
        for (std::string& emphasized_word : paragraph.emphasized_words) {
            json_pdf_paragraph["keywords"] += emphasized_word;
        }
        json_pdf_section["paragraphs"] += json_pdf_paragraph;
    }

    if(current_node.sub_sections) {
        for (PDF_Section_Node& node : current_node.sub_sections.value()) {
            json_pdf_section["subnodes"] += add_json_node(node, id);
        }
    }

    return json_pdf_section;
}

std::string format_pdf_document_tree(PDF_Section_Node &doc_root)
{
    // present as tree
    unsigned int start_id = 0;
    nlohmann::json json_pdf_document = add_json_node(doc_root, start_id);
    return json_pdf_document.dump();
}
