#include <xlnt/common/zip_file.hpp>
#include <xlnt/styles/border.hpp>
#include <xlnt/styles/fill.hpp>
#include <xlnt/styles/font.hpp>
#include <xlnt/styles/named_style.hpp>
#include <xlnt/styles/number_format.hpp>
#include <xlnt/styles/style.hpp>
#include <xlnt/reader/style_reader.hpp>

#include "detail/include_pugixml.hpp"

namespace xlnt {

protection read_protection(pugi::xml_node node)
{
    auto prot_type_from_string = [](const std::string &type_string)
    {
        if(type_string == "true") return protection::type::protected_;
        else if(type_string == "inherit") return protection::type::inherit;
        return protection::type::unprotected;
    };
 
    protection prot;
    prot.set_locked(prot_type_from_string(node.attribute("locked").as_string()));
    prot.set_hidden(prot_type_from_string(node.attribute("hidden").as_string()));
    return prot;
}
    
alignment read_alignment(pugi::xml_node node)
{
    alignment align;
    
    align.set_wrap_text(node.attribute("wrapText").as_int() != 0);
    
    bool has_vertical = node.attribute("vertical") != nullptr;
    std::string vertical = has_vertical ? node.attribute("vertical").as_string() : "";
    
    if(has_vertical)
    {
        if(vertical == "bottom")
        {
            align.set_vertical(alignment::vertical_alignment::bottom);
        }
        else if(vertical =="center")
        {
            align.set_vertical(alignment::vertical_alignment::center);
        }
        else if(vertical =="justify")
        {
            align.set_vertical(alignment::vertical_alignment::justify);
        }
        else if(vertical =="top")
        {
            align.set_vertical(alignment::vertical_alignment::top);
        }
        else
        {
            throw "unknown alignment";
        }
    }
    
    bool has_horizontal = node.attribute("horizontal") != nullptr;
    std::string horizontal = has_horizontal ? node.attribute("horizontal").as_string() : "";
    
    if(has_horizontal)
    {
        if(horizontal == "left")
        {
            align.set_horizontal(alignment::horizontal_alignment::left);
        }
        else if(horizontal == "center")
        {
            align.set_horizontal(alignment::horizontal_alignment::center);
        }
        else if(horizontal == "center-continuous")
        {
            align.set_horizontal(alignment::horizontal_alignment::center_continuous);
        }
        else if(horizontal == "right")
        {
            align.set_horizontal(alignment::horizontal_alignment::right);
        }
        else if(horizontal == "justify")
        {
            align.set_horizontal(alignment::horizontal_alignment::justify);
        }
        else if(horizontal == "general")
        {
            align.set_horizontal(alignment::horizontal_alignment::general);
        }
        else
        {
            throw "unknown alignment";
        }
    }
    
    return align;
}

style style_reader::read_style(pugi::xml_node stylesheet_node, pugi::xml_node xf_node)
{
    style s;
    
    s.apply_number_format(xf_node.attribute("applyNumberFormat").as_bool());
    s.number_format_id_ = xf_node.attribute("numFmtId").as_int();
    
    bool builtin_format = true;
    
    for(auto num_fmt : number_formats_)
    {
        if(num_fmt.get_id() == s.get_number_format_id())
        {
            s.number_format_ = num_fmt;
            builtin_format = false;
            break;
        }
    }
    
    if(builtin_format)
    {
        s.number_format_ = number_format::from_builtin_id(s.get_number_format_id());
    }
    
    s.apply_font(xf_node.attribute("applyFont").as_bool());
    s.font_id_ = xf_node.attribute("fontId") != nullptr ? xf_node.attribute("fontId").as_int() : 0;
    s.font_ = fonts_[s.font_id_];
    
    s.apply_fill(xf_node.attribute("applyFill").as_bool());
    s.fill_id_ = xf_node.attribute("fillId").as_int();
    s.fill_ = fills_[s.fill_id_];
    
    s.apply_border(xf_node.attribute("applyBorder").as_bool());
    s.border_id_ = xf_node.attribute("borderId").as_int();
    s.border_ = borders_[s.border_id_];
    
    s.apply_protection(xf_node.attribute("protection") != nullptr);
    
    if(s.protection_apply_)
    {
        auto inline_protection = read_protection(xf_node.child("protection"));
        s.protection_ = inline_protection;
    }
    
    s.apply_alignment(xf_node.child("alignment") != nullptr);
    
    if(s.alignment_apply_)
    {
        auto inline_alignment = read_alignment(xf_node.child("alignment"));
        s.alignment_ = inline_alignment;
    }
    
    return s;
}

style_reader::style_reader(workbook &wb)
{

}

void style_reader::read_styles(zip_file &archive)
{
    auto xml = archive.read("xl/styles.xml");
    
    pugi::xml_document doc;
    doc.load_string(xml.c_str());
    
    auto root = doc.root();
    auto stylesheet_node = root.child("styleSheet");
    
    read_borders(stylesheet_node.child("borders"));
    read_fills(stylesheet_node.child("fills"));
    read_fonts(stylesheet_node.child("fonts"));
    read_number_formats(stylesheet_node.child("numFmts"));
    
    auto cell_xfs_node = stylesheet_node.child("cellXfs");
    
    for(auto xf_node : cell_xfs_node.children("xf"))
    {
        styles_.push_back(read_style(stylesheet_node, xf_node));
        styles_.back().id_ = styles_.size() - 1;
    }

    read_color_index();
    read_dxfs();
    read_cell_styles();
    read_named_styles(root.child("namedStyles"));
}

void style_reader::read_number_formats(pugi::xml_node num_fmts_node)
{
    for(auto num_fmt_node : num_fmts_node.children("numFmt"))
    {
        number_format nf;
        
        nf.set_format_string(num_fmt_node.attribute("formatCode").as_string());
        if(nf.get_format_string() == "GENERAL")
        {
            nf.set_format_string("General");
        }
        nf.set_id(num_fmt_node.attribute("numFmtId").as_int());
        
        number_formats_.push_back(nf);
    }
}

void style_reader::read_color_index()
{
    
}
    
void style_reader::read_dxfs()
{
    
}

void style_reader::read_fonts(pugi::xml_node fonts_node)
{
    for(auto font_node : fonts_node)
    {
        font new_font;
        
        new_font.set_size(font_node.child("sz").attribute("val").as_int());
        new_font.set_name(font_node.child("name").attribute("val").as_string());
        
        if(font_node.child("color").attribute("theme") != nullptr)
        {
            new_font.set_color(color(color::type::theme, font_node.child("color").attribute("theme").as_ullong()));
        }
        else if(font_node.child("color").attribute("indexed") != nullptr)
        {
            new_font.set_color(color(color::type::indexed, font_node.child("color").attribute("indexed").as_ullong()));
        }
        
        if(font_node.child("family") != nullptr)
        {
            new_font.set_family(font_node.child("family").attribute("val").as_int());
        }
        
        if(font_node.child("scheme") != nullptr)
        {
            new_font.set_scheme(font_node.child("scheme").attribute("val").as_string());
        }
        
        if(font_node.child("b") != nullptr)
        {
            new_font.set_bold(font_node.child("b").attribute("val").as_bool());
        }
        
        fonts_.push_back(new_font);
    }
}

void style_reader::read_fills(pugi::xml_node fills_node)
{
    auto pattern_fill_type_from_string = [](const std::string &fill_type)
    {
        if(fill_type == "none") return fill::pattern_type::none;
        if(fill_type == "solid") return fill::pattern_type::solid;
        if(fill_type == "gray125") return fill::pattern_type::gray125;
        throw std::runtime_error("invalid fill type");
    };
    
    for(auto fill_node : fills_node)
    {
        fill new_fill;
        
        if(fill_node.child("patternFill") != nullptr)
        {
            auto pattern_fill_node = fill_node.child("patternFill");
            new_fill.set_type(fill::type::pattern);
            std::string pattern_type_string = pattern_fill_node.attribute("patternType").as_string();
            auto pattern_type = pattern_fill_type_from_string(pattern_type_string);
            new_fill.set_pattern_type(pattern_type);
            
            auto bg_color_node = pattern_fill_node.child("bgColor");
            
            if(bg_color_node != nullptr)
            {
                if(bg_color_node.attribute("indexed") != nullptr)
                {
                    new_fill.set_background_color(color(color::type::indexed, bg_color_node.attribute("indexed").as_ullong()));
                }
            }
            
            auto fg_color_node = pattern_fill_node.child("fgColor");
            
            if(fg_color_node != nullptr)
            {
                if(fg_color_node.attribute("indexed") != nullptr)
                {
                    new_fill.set_foreground_color(color(color::type::indexed, fg_color_node.attribute("indexed").as_ullong()));
                }
            }
        }

        fills_.push_back(new_fill);
    }
}

void style_reader::read_borders(pugi::xml_node borders_node)
{
    for(auto border_node : borders_node)
    {
        border new_border;
        
        if(border_node.child("left") != nullptr)
        {
            new_border.left_assigned = true;
            auto left_node = border_node.child("left");
            
            if(left_node.attribute("style") != nullptr)
            {
                if(left_node.attribute("style").as_string() == std::string("thin"))
                {
                    new_border.left.set_border_style(border_style::thin);
                }
                else
                {
                    throw std::runtime_error("unknown border style");
                }
            }
            
            auto color_node = left_node.child("color");
            
            if(color_node != nullptr)
            {
                if(color_node.attribute("indexed") != nullptr)
                {
                    new_border.left.set_color(side::color_type::indexed, color_node.attribute("indexed").as_int());
                }
                else if(color_node.attribute("theme") != nullptr)
                {
                    new_border.left.set_color(side::color_type::theme, color_node.attribute("theme").as_int());
                }
                else
                {
                    throw std::runtime_error("invalid color type");
                }
            }
        }
        if(border_node.child("right") != nullptr)
        {
            new_border.right_assigned = true;
            auto right_node = border_node.child("right");
            
            if(right_node.attribute("style") != nullptr)
            {
                if(right_node.attribute("style").as_string() == std::string("thin"))
                {
                    new_border.right.set_border_style(border_style::thin);
                }
                else
                {
                    throw std::runtime_error("unknown border style");
                }
            }
            
            auto color_node = right_node.child("color");
            
            if(color_node != nullptr)
            {
                if(color_node.attribute("indexed") != nullptr)
                {
                    new_border.right.set_color(side::color_type::indexed, color_node.attribute("indexed").as_int());
                }
                else if(color_node.attribute("theme") != nullptr)
                {
                    new_border.right.set_color(side::color_type::theme, color_node.attribute("theme").as_int());
                }
                else
                {
                    throw std::runtime_error("invalid color type");
                }
            }
        }
        if(border_node.child("top") != nullptr)
        {
            new_border.top_assigned = true;
            auto top_node = border_node.child("top");
            
            if(top_node.attribute("style") != nullptr)
            {
                if(top_node.attribute("style").as_string() == std::string("thin"))
                {
                    new_border.top.set_border_style(border_style::thin);
                }
                else
                {
                    throw std::runtime_error("unknown border style");
                }
            }
            
            auto color_node = top_node.child("color");
            
            if(color_node != nullptr)
            {
                if(color_node.attribute("indexed") != nullptr)
                {
                    new_border.top.set_color(side::color_type::indexed, color_node.attribute("indexed").as_int());
                }
                else if(color_node.attribute("theme") != nullptr)
                {
                    new_border.top.set_color(side::color_type::theme, color_node.attribute("theme").as_int());
                }
                else
                {
                    throw std::runtime_error("invalid color type");
                }
            }
        }
        if(border_node.child("bottom") != nullptr)
        {
            new_border.bottom_assigned = true;
            auto bottom_node = border_node.child("bottom");
            
            if(bottom_node.attribute("style") != nullptr)
            {
                if(bottom_node.attribute("style").as_string() == std::string("thin"))
                {
                    new_border.bottom.set_border_style(border_style::thin);
                }
                else
                {
                    throw std::runtime_error("unknown border style");
                }
            }
            
            auto color_node = bottom_node.child("color");
            
            if(color_node != nullptr)
            {
                if(color_node.attribute("indexed") != nullptr)
                {
                    new_border.bottom.set_color(side::color_type::indexed, color_node.attribute("indexed").as_int());
                }
                else if(color_node.attribute("theme") != nullptr)
                {
                    new_border.bottom.set_color(side::color_type::theme, color_node.attribute("theme").as_int());
                }
                else
                {
                    throw std::runtime_error("invalid color type");
                }
            }
        }
        if(border_node.child("diagonal") != nullptr)
        {
            new_border.diagonal_assigned = true;
            auto diagonal_node = border_node.child("diagonal");
            
            if(diagonal_node.attribute("style") != nullptr)
            {
                if(diagonal_node.attribute("style").as_string() == std::string("thin"))
                {
                    new_border.diagonal.set_border_style(border_style::thin);
                }
                else
                {
                    throw std::runtime_error("unknown border style");
                }
            }
            
            auto color_node = diagonal_node.child("color");
            
            if(color_node != nullptr)
            {
                if(color_node.attribute("indexed") != nullptr)
                {
                    new_border.diagonal.set_color(side::color_type::indexed, color_node.attribute("indexed").as_int());
                }
                else if(color_node.attribute("theme") != nullptr)
                {
                    new_border.diagonal.set_color(side::color_type::theme, color_node.attribute("theme").as_int());
                }
                else
                {
                    throw std::runtime_error("invalid color type");
                }
            }
        }
        
        borders_.push_back(new_border);
    }
}

void style_reader::read_named_styles(pugi::xml_node named_styles_node)
{
}
    
void style_reader::read_style_names()
{
}
    
void style_reader::read_cell_styles()
{
    
}
    
void style_reader::read_xfs()
{
    
}
    
void style_reader::read_style_table()
{
    
}
    
} // namespace xlnt