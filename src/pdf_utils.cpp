#include "pdf_utils.hpp"

#include <mupdf/fitz.h>

std::optional<PDF_Document> parse_pdf_file(std::string file_path) {
    PDF_Document pdf_document;
    int page_number, page_count;
    fz_context* ctx;
    fz_document* doc;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) {
        fprintf(stderr, "cannot create mupdf context\n");
        return std::nullopt;
    }

    /* Register the default file types to handle. */
    fz_try(ctx)
        fz_register_document_handlers(ctx);
    fz_catch(ctx) {
        fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
        fz_drop_context(ctx);
        return std::nullopt;
    }

    /* Open the document. */
    fz_try(ctx)
        doc = fz_open_document(ctx, file_path.c_str());
    fz_catch(ctx) {
        fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
        fz_drop_context(ctx);
        return std::nullopt;
    }

    /* Count the number of pages. */
    fz_try(ctx)
        page_count = fz_count_pages(ctx, doc);
    fz_catch(ctx)
    {
        fprintf(stderr, "cannot count number of pages: %s\n", fz_caught_message(ctx));
        fz_drop_document(ctx, doc);
        fz_drop_context(ctx);
        return std::nullopt;
    }

    for (page_number = 0; page_number < page_count; ++page_number) {
        fz_try(ctx)

            int i = 0;
//            pix = fz_new_pixmap_from_page_number(ctx, doc, page_number, &ctm, fz_device_rgb(ctx), 0);
        fz_catch(ctx)
        {
            fprintf(stderr, "cannot render page: %s\n", fz_caught_message(ctx));
            fz_drop_document(ctx, doc);
            fz_drop_context(ctx);
            return std::nullopt;
        }
    }

    /* Clean up. */
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);
    return pdf_document;
}
