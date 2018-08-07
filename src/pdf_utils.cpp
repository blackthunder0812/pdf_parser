#include "pdf_utils.hpp"
#include "string_utils.hpp"
#include <iostream>
#include <vector>
#include <regex>
#include <sstream>

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
            fz_font* prev_ch_font = nullptr;

            for (block = text->first_block; block; block = block->next) {
                next_block = block->next;

                if (block->type == FZ_STEXT_BLOCK_TEXT) { // only text blocks have lines, image blocks do not have lines
                    TextBlockInformation text_block_information;
                    std::optional<std::string> title_prefix = std::nullopt;
                    std::optional<fz_font*> title_font = std::nullopt;
                    std::optional<double> title_indent = std::nullopt, title_baseline = std::nullopt;
                    std::stringstream partial_paragraph_content_string_stream;
                    std::stringstream emphasized_word_string_stream;
                    bool parsing_emphasized_word = false;

                    for (line = block->u.t.first_line; line; line = line->next) {
                        next_line = line->next;

                        for (ch = line->first_char; ch; ch = ch->next) {
                            next_ch = ch->next;

                            std::string character = UnicodeToUTF8(ch->c);

                            // linhlt: temporary fix
                            if (character.compare("“") == 0 ||
                                character.compare("”") == 0) {
                                character = "\"";
                            }

                            if (prev_ch_font && parsing_emphasized_word) {  // just need to compare to font of previous character
                                if (ch->font == prev_ch_font) { // same as previous character, continue to add to emphasized word
                                    emphasized_word_string_stream << character;
                                } else {
                                    // add emphasized word to list
                                    std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                                    if (trimmed_string.length() > 0) {
                                        text_block_information.emphasized_words.push_back(trimmed_string);
                                    }
                                    // reset string stream
                                    emphasized_word_string_stream.str(std::string());
                                    parsing_emphasized_word = false;

                                    if (fz_font_is_bold(ctx, ch->font) || fz_font_is_italic(ctx, ch->font)) {
                                        parsing_emphasized_word = true;
                                        emphasized_word_string_stream << character;
                                    }
                                }
                            } else {
                                if (fz_font_is_bold(ctx, ch->font) || fz_font_is_italic(ctx, ch->font)) {
                                    parsing_emphasized_word = true;
                                    // first time this occured
                                    if (!title_prefix) {
                                        if (!title_font) {
                                            title_font = ch->font;
                                        }

                                        if (!partial_paragraph_content_string_stream.str().empty() &&
                                            text_block_information.emphasized_words.empty()) {
                                            title_prefix = partial_paragraph_content_string_stream.str();
                                        }
                                    }
                                    emphasized_word_string_stream << character;
                                } else if (parsing_emphasized_word) { // end of parsing emphasized word
                                    std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                                    if (trimmed_string.length() > 0) {
                                        text_block_information.emphasized_words.push_back(trimmed_string);
                                    }
                                    emphasized_word_string_stream.str(std::string());
                                    parsing_emphasized_word = false;
                                }
                            }

                            // add character to partial paragraph content
                            partial_paragraph_content_string_stream << character;

                            prev_ch_font = ch->font;

                            prev_ch = ch;
                        };

                        prev_line = line;
                    }

                    text_block_information.partial_paragraph_content = partial_paragraph_content_string_stream.str();

                    // if emphasized_word is in the end of partial_paragraph
                    std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                    if (parsing_emphasized_word &&
                        trimmed_string.length() > 0) {
                        text_block_information.emphasized_words.push_back(trimmed_string);
                    }

                    if (!text_block_information.emphasized_words.empty() &&
                        text_block_information.emphasized_words.front().length() < TITLE_MAX_LENGTH) {
                        std::smatch title_prefix_match_result;

                        if (title_prefix) {
                            // case 1: prefix is in following format: bullet/numbering space single/double quote
                            // step 1: find first word, using regex to match, check if it is bullet or numbering
                            unsigned int pos = 0;
                            std::string_view title_prefix_view(title_prefix.value());;
                            size_t p_length = title_prefix_view.length();
                            for (unsigned int i = 0; i < p_length; ++i) {
                                if (std::isspace(title_prefix_view[i])) {
                                    pos = i;
                                    break;
                                }
                            }

                            if (pos > 0) {
                                std::string first_word_title_prefix_view(title_prefix_view.substr(0, pos));
                                PDF_Title_Format title_format;
                                bool has_title_format = true;

                                // bullet match
                                if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("[•\\*\\+\\-]"))) {
                                    title_format.prefix = PDF_Title_Format::PREFIX::BULLET;
                                } // numbering using latin characters (a) (b) (c)
                                else if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([a-z]{2}\\)"))) {
                                    title_format.prefix = PDF_Title_Format::PREFIX::ALPHABET_LOWERCASE_NUMBERING;
                                }  // numbering using latin characters (A) (B) (C)
                                else if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([A-Z]{2}\\)"))) {
                                    title_format.prefix = PDF_Title_Format::PREFIX::ALPHABET_UPPERCASE_NUMBERING;
                                } // numbering using roman numerals (i), longest presentation might be (xviii)
                                else if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([ivx]{1,5}\\)"))) {
                                    title_format.prefix = PDF_Title_Format::PREFIX::ROMAN_NUMBERING;
                                } // numbering using number with dots ex 1 2 3 or 1. 2. 3. or 1.1 1.2 1.3 or 1.1. 1.2. 1.3.
                                else if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\d+(\\.\\d+)*\\.?"))) {
                                    title_format.prefix = PDF_Title_Format::PREFIX::NUMBER_DOT_NUMBERING;
                                } else { // not bullet or numbering
                                    has_title_format = false;
                                }

                                // check if the rest is single/double quouted
                                std::string_view the_rest_title_prefix_view(title_prefix_view.substr(pos + 1, p_length - pos));
                                if (the_rest_title_prefix_view.empty()) {
                                    title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::NONE;
                                } else if (the_rest_title_prefix_view.compare("'") == 0 &&
                                           text_block_information.partial_paragraph_content[text_block_information.emphasized_words.front().length() + title_prefix_view.length()] == '\'') {
                                    title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::SINGLE_QUOTE;
                                } else if (the_rest_title_prefix_view.compare("\"") == 0 &&
                                           text_block_information.partial_paragraph_content[text_block_information.emphasized_words.front().length() + title_prefix_view.length()] == '\"') {
                                    title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                                } else {
                                    has_title_format = false;
                                }

                                if (has_title_format) {
                                    text_block_information.title_format = std::move(title_format);
                                }
                            } else { // no space in prefix
                                if (title_prefix_view.compare("'") == 0 &&
                                    text_block_information.partial_paragraph_content[text_block_information.emphasized_words.front().length() + 1] == '\'') {
                                    PDF_Title_Format title_format;
                                    title_format.prefix = PDF_Title_Format::PREFIX::NONE;
                                    title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::SINGLE_QUOTE;
                                    text_block_information.title_format = std::move(title_format);
                                } else if (title_prefix_view.compare("\"") == 0 &&
                                           text_block_information.partial_paragraph_content[text_block_information.emphasized_words.front().length() + 1] == '\"') {
                                    PDF_Title_Format title_format;
                                    title_format.prefix = PDF_Title_Format::PREFIX::NONE;
                                    title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                                    text_block_information.title_format = std::move(title_format);
                                }
                            }

                            if (text_block_information.title_format) {
                                text_block_information.partial_paragraph_content.erase(0, text_block_information.emphasized_words.front().length() + title_prefix_view.length());
                                if (text_block_information.title_format->emphasize_style > PDF_Title_Format::EMPHASIZE_STYLE::NONE) {
                                    text_block_information.partial_paragraph_content.erase(0, 1); // single or double quote, so remove extra one more character
                                }
                            }
                        }  else {
                            // case 2: no prefix: first emphasize word is in begining of the block, the character after first emphasized word must be colon or space
                            size_t pos = text_block_information.emphasized_words.front().length();
                            size_t p_length = text_block_information.partial_paragraph_content.length();
                            if (pos == p_length) {
                                PDF_Title_Format title_format;
                                title_format.prefix = PDF_Title_Format::PREFIX::NONE;
                                title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::NONE;
                                title_format.same_line_with_content = false;
                                text_block_information.title_format = std::move(title_format);

                                // cut title out of content
                                text_block_information.partial_paragraph_content = "";
                            } else if (pos < p_length &&
                                       (text_block_information.partial_paragraph_content[pos] == ' ' ||
                                        text_block_information.partial_paragraph_content[pos] == ':' ||
                                        text_block_information.partial_paragraph_content[pos] == '.')) {
                                PDF_Title_Format title_format;
                                title_format.prefix = PDF_Title_Format::PREFIX::NONE;
                                title_format.emphasize_style = PDF_Title_Format::EMPHASIZE_STYLE::NONE;
                                text_block_information.title_format = std::move(title_format);

                                // cut title out of content
                                text_block_information.partial_paragraph_content.erase(0, pos + 1);
                            }
                        }
                    }





                    std::cout << "Emphasized words: ";
                    for (auto it = text_block_information.emphasized_words.begin(); it != text_block_information.emphasized_words.end(); ++it ) {
                        std::cout << "\"" << *it << "\", ";
                    }
                    std::cout << "\nContent: " << text_block_information.partial_paragraph_content << std::endl;
                    std::cout << "\n---------------------------------------------------------\n" << std::endl;



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
