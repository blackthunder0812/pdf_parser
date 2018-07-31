#include "pdf_utils.hpp"

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;
    auto pdf_doc = parse_pdf_file("/home/sonbn/Workspace/Cinnamon/proj_nochu_test_data/01.pdf");
    return 0;
}
