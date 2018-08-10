#include <iostream>
#include "pdf_utils.hpp"
#include "string_utils.hpp"

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    std::optional<PDF_Document> pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/01.pdf");
    if (pdf_doc) {
        PDF_Document pdf_document = pdf_doc.value();
        PDF_Section root_section;
        root_section.id = 0;
        root_section.title = pdf_document.document_info.title;
        root_section.paragraphs = pdf_document.prefix_content;
        PDF_Section_Node doc_root = construct_document_tree(pdf_document, root_section);
        std::cout << format_pdf_document_tree(doc_root);
    }
    return 0;
}
