#include <iostream>
#include "pdf_utils.hpp"
#include "string_utils.hpp"

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    std::optional<PDF_Document> pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/01.pdf");
    if (pdf_doc) {
        PDF_Section_Node doc_root = construct_document_tree(pdf_doc.value());
        std::cout << format_pdf_document_tree(doc_root);
    }
    return 0;
}
