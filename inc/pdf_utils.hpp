#pragma once

#include <string>
#include <optional>
#include <list>

#include <mupdf/fitz.h>

#ifndef TITLE_FORMAT_INDENT_DELTA
#define TITLE_FORMAT_INDENT_DELTA 1.0
#endif

#ifndef TITLE_MAX_LENGTH
#define TITLE_MAX_LENGTH 200
#endif

struct PDF_Title_Format {
        static const double INDENT_DELTA_THRESHOLD;
        enum class CASE {ALL_UPPER, FIRST_ONLY_UPPER};
        enum class PREFIX {NONE, BULLET, ROMAN_NUMBERING, NUMBER_DOT_NUMBERING, ALPHABET_LOWERCASE_NUMBERING, ALPHABET_UPPERCASE_NUMBERING};
        enum class EMPHASIZE_STYLE {NONE, SINGLE_QUOTE, DOUBLE_QUOTE};

        fz_font *title_font;
        CASE title_case = CASE::FIRST_ONLY_UPPER;
        PREFIX prefix = PREFIX::NONE;
        EMPHASIZE_STYLE emphasize_style = EMPHASIZE_STYLE::NONE;
        unsigned long numbering_level = 0;
        bool same_line_with_content = true;
        double indent = 0;

        PDF_Title_Format();
        PDF_Title_Format(const PDF_Title_Format& other);
        PDF_Title_Format(PDF_Title_Format&& other);
        PDF_Title_Format& operator=(const PDF_Title_Format& other);
        PDF_Title_Format& operator=(PDF_Title_Format&& other);
        bool operator==(const PDF_Title_Format& title_format);
        bool operator!=(const PDF_Title_Format& title_format);

        friend std::ostream& operator<<(std::ostream& os, const PDF_Title_Format& tf);
};

struct PDF_Paragraph {
    std::string paragraph;
    std::list<std::string> emphasized_words;
    unsigned int page;
    fz_rect bbox;
};

struct PDF_Section {
    unsigned int id;
    std::string title;
    PDF_Title_Format title_format;
    std::list<PDF_Paragraph> paragraphs;
    std::optional<std::list<PDF_Section>> sub_sections;
};

struct PDF_Document_Info {
    struct PDF_Document_Version {
        unsigned int major_version;
        unsigned int minor_version;
    } version;
    std::string title;
    std::string author;
    std::string subject;
    std::string keywords;
    std::string creator;
    std::string producer;
    std::string create_date;
    std::string modified_date;
};

struct PDF_Document {
    PDF_Document_Info document_info;
    std::optional<std::list<PDF_Paragraph>> prefix_content;
    std::list<PDF_Section> sections;
};

struct TextBlockInformation {
    std::optional<PDF_Title_Format> title_format;
    std::list<std::string> emphasized_words;
    std::string partial_paragraph_content;
};

// return nullopt if cant read pdf document
std::optional<PDF_Document> parse_pdf_file(std::string file_path);
