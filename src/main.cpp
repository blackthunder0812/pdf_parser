#include "pdf_utils.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    std::optional<PDF_Document> pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/01.pdf");
    if (pdf_doc) {
        PDF_Document_Info& pdf_doc_info = pdf_doc.value().document_info;
        std::cout << "Format:            " << pdf_doc_info.format << std::endl
                  << "Encryption:        " << pdf_doc_info.encryption << std::endl
                  << "Title:             " << pdf_doc_info.title << std::endl
                  << "Author:            " << pdf_doc_info.author << std::endl
                  << "Subject:           " << pdf_doc_info.subject << std::endl
                  << "Keywords:          " << pdf_doc_info.keywords << std::endl
                  << "Creator:           " << pdf_doc_info.creator << std::endl
                  << "Producer:          " << pdf_doc_info.producer << std::endl
                  << "Creation Date:     " << pdf_doc_info.create_date << std::endl
                  << "Modification Date: " << pdf_doc_info.modified_date << std::endl;

        for (PDF_Section& section : pdf_doc.value().sections) {
            std::cout << "\n------------------------------------------------\n" << section.title << std::endl;
            for (PDF_Paragraph& p : section.paragraphs) {
                std::cout << p.paragraph << std::endl;
            }
        }
    }
    return 0;
}
