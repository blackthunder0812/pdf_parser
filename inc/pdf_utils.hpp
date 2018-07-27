#pragma once

#include <string>
#include <optional>
#include <list>
#include <mupdf/fitz/geometry.h>

struct PDF_Title_Format {

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

std::optional<PDF_Document> parse_pdf_file(std::string file_path);
