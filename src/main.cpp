#include "pdf_utils.hpp"

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    std::optional<PDF_Document> pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/02.pdf");
    return 0;
}
