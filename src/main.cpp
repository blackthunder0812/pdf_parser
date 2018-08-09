#include "pdf_utils.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    std::optional<PDF_Document> pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/01.pdf");
    if (pdf_doc) {
        for (PDF_Section& section : pdf_doc.value().sections) {
            std::cout << section.title << std::endl;
            for (PDF_Paragraph& p : section.paragraphs) {
                std::cout << p.paragraph << std::endl;
            }
        }
    }
    return 0;
}
