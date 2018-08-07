#include "pdf_utils.hpp"
#include "string_utils.hpp"
#include <iostream>
#include <vector>

PDF_Title_Format::PDF_Title_Format() :
    title_font(nullptr) {

}

PDF_Title_Format::PDF_Title_Format(const PDF_Title_Format& other) :
    title_font(other.title_font),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    numbering_level(other.numbering_level),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

PDF_Title_Format::PDF_Title_Format(PDF_Title_Format&& other) :
    title_font(other.title_font),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    numbering_level(other.numbering_level),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

PDF_Title_Format& PDF_Title_Format::operator=(const PDF_Title_Format& other) {
    title_font = other.title_font;
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    numbering_level = other.numbering_level;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}

PDF_Title_Format& PDF_Title_Format::operator=(PDF_Title_Format&& other) {
    title_font = other.title_font;
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    numbering_level = other.numbering_level;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}

bool PDF_Title_Format::operator==(const PDF_Title_Format& title_format) {
    return title_font == title_format.title_font &&
           title_case == title_format.title_case &&
           prefix == title_format.prefix &&
           emphasize_style == title_format.emphasize_style &&
           numbering_level == title_format.numbering_level &&
           !(same_line_with_content ^ title_format.same_line_with_content);
}

bool PDF_Title_Format::operator!=(const PDF_Title_Format& title_format) {
    return title_font != title_format.title_font ||
           title_case != title_format.title_case ||
           prefix != title_format.prefix ||
           emphasize_style != title_format.emphasize_style ||
           numbering_level != title_format.numbering_level ||
           (same_line_with_content ^ title_format.same_line_with_content);
}

std::ostream& operator<<(std::ostream& os, const PDF_Title_Format& tf) {
    os << "\nFont addr: " << (void*)tf.title_font
       << "\nTitle case: " << static_cast<unsigned int>(tf.title_case)
       << "\nTitle prefix: " << static_cast<unsigned int>(tf.prefix)
       << "\nEmphasize style: " << static_cast<unsigned int>(tf.emphasize_style)
       << "\nNumbering level: " << tf.numbering_level
       << "\nIs same line with content: " << tf.same_line_with_content
       << "\nIndent: " << tf.indent << std::endl;
    return os;
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

            fz_stext_block* block = nullptr, *prev_block = nullptr, *next_block = nullptr;
            fz_stext_line* line = nullptr, *prev_line = nullptr, *next_line = nullptr;
            fz_stext_char* ch = nullptr, *prev_ch = nullptr, *next_ch = nullptr;

            for (block = text->first_block; block; block = block->next) {
                next_block = block->next;

                if (block->type == FZ_STEXT_BLOCK_TEXT) { // only text blocks have lines, image blocks do not have lines
                    for (line = block->u.t.first_line; line; line = line->next) {
                        next_line = line->next;

                        for (ch = line->first_char; ch; ch = ch->next) {
                            next_ch = ch->next;

//                            std::cout << (void*)(ch) << " " << (void*)(prev_ch) << std::endl;
//                            std::cout << UnicodeToUTF8(ch->c) << " " << (prev_ch ? UnicodeToUTF8(prev_ch->c) : " ") << std::endl;

                            if (fz_font_is_bold(ctx, ch->font) || fz_font_is_italic(ctx, ch->font)) {
                                std::cout << UnicodeToUTF8(ch->c);
                            }

                            prev_ch = ch;
                        }
                        std::cout << std::endl;

                        prev_line = line;
                    }
                    std::cout << std::endl;
                }

                prev_block = block;
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
