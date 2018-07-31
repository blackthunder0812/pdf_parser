#include "pdf_utils.hpp"
#include <mupdf/fitz.h>
#include <iostream>

inline std::string UnicodeToUTF8(int codepoint) {
    std::string out;
    if (codepoint <= 0x7f)
        out.append(1, static_cast<char>(codepoint));
    else if (codepoint <= 0x7ff) {
        out.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else if (codepoint <= 0xffff) {
        out.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else {
        out.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    return out;
}

std::optional<PDF_Document> parse_pdf_file(std::string file_path) {
    PDF_Document pdf_document;
    int page_number, page_count = 0;
    fz_context* ctx = nullptr;
    fz_document* doc = nullptr;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) {
        fprintf(stderr, "cannot create mupdf context\n");
        return std::nullopt;
    }

    /* Register the default file types to handle. */
    fz_try(ctx) {
        fz_register_document_handlers(ctx);
    } fz_catch(ctx) {
        fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
        fz_drop_context(ctx);
        return std::nullopt;
    }

    /* Open the document. */
    fz_try(ctx) {
        doc = fz_open_document(ctx, file_path.c_str());
    } fz_catch(ctx) {
        fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
        fz_drop_context(ctx);
        return std::nullopt;
    }

    /* Count the number of pages. */
    fz_try(ctx) {
        page_count = fz_count_pages(ctx, doc);
    } fz_catch(ctx) {
        fprintf(stderr, "cannot count number of pages: %s\n", fz_caught_message(ctx));
        fz_drop_document(ctx, doc);
        fz_drop_context(ctx);
        return std::nullopt;
    }

    for (page_number = 0; page_number < page_count; ++page_number) {

        fz_page* page = fz_load_page(ctx, doc, page_number);
        fz_device* dev = nullptr;
        fz_var(dev);

        fz_rect mediabox;
        fz_try(ctx) {
            mediabox = fz_bound_page(ctx, page);
        } fz_catch(ctx) {
            fprintf(stderr, "cannot get mediabox of page %d: %s\n", page_number, fz_caught_message(ctx));
            fz_drop_page(ctx, page);
            return std::nullopt;
        }

        fz_stext_page* text = nullptr;
        fz_var(text);

        fz_try(ctx) {
            fz_stext_options stext_options;
            stext_options.flags = 0;
            text = fz_new_stext_page(ctx, mediabox);
            dev = fz_new_stext_device(ctx,  text, &stext_options);
            fz_run_page(ctx, page, dev, fz_identity, nullptr);
            fz_close_device(ctx, dev);
            fz_drop_device(ctx, dev);
            dev = nullptr;

            fz_stext_block* block;
            fz_stext_line* line;
            fz_stext_char* ch;

            for (block = text->first_block; block; block = block->next) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    for (line = block->u.t.first_line; line; line = line->next) {
                        for (ch = line->first_char; ch; ch = ch->next) {
                            std::cout << UnicodeToUTF8(ch->c);
                        }
                        std::cout << std::endl;
                    }
                    std::cout << std::endl;
                }
            }


        } fz_always(ctx) {
            fz_drop_device(ctx, dev);
            fz_drop_stext_page(ctx, text);
        } fz_catch(ctx) {
            fprintf(stderr, "render page %d error: %s\n", page_number, fz_caught_message(ctx));
            fz_drop_page(ctx, page);
            fz_rethrow(ctx);
            return std::nullopt;
        }

        fz_drop_page(ctx, page);
    }

    /* Clean up. */
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);
    return pdf_document;
}
