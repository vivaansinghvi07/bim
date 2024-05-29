from string import ascii_uppercase
from dataclasses import dataclass

@dataclass
class SyntaxInfo:
        file_type: str
        abbreviation: str
        string_chars: str
        short_comment: str
        long_comment_start: str
        long_comment_end: str

def fill_keyword_list() -> list[list[tuple[str, str]]]:
        keyword = input("Enter keywords below: ")
        all_keywords = []
        while keyword:
                all_keywords.append(keyword)
                keyword = input()
        keywords_by_letter = [[] for _ in range(27)]

        for keyword in all_keywords:
                kw_type = input(f"Enter the type of the keyword \"{keyword}\" [d/t/c/m/o/f]: ")
                while kw_type not in "dtcmof" or not kw_type:
                        kw_type = input("Enter a valid keyword type. Refer to the src/display/syntax.h file for the types: ")
                if (c := keyword[0].upper()) in ascii_uppercase:
                        keywords_by_letter[ord(c) - ord('A')].append((keyword, kw_type))
                else:
                        keywords_by_letter[26].append((keyword, kw_type))
        return keywords_by_letter

def get_keyword_type(c: str) -> str:
        match c:
                case 'd': return "KW_DECL"
                case 't': return "KW_TYPE"
                case 'c': return "KW_CTRL_FLOW"
                case 'm': return "KW_MODIFIER"
                case 'o': return "KW_CONST"
                case 'f': return "KW_SPECIAL_FUNC"
                case _: raise ValueError(f"Invalid abbreviation for keyword type: {c}")

def generate_keyword_string(keywords: list[list[tuple[str, str]]], abbr: str) -> str:
        code = ""
        for c, kw_list in enumerate(keywords):
                if not kw_list:
                        continue
                line = f"static keyword_t FT_{abbr.upper()}_{'_' if c == 26 else ascii_uppercase[c]}[] = {{"
                for kw, typ in kw_list:
                        line += f"{{\"{kw}\", {get_keyword_type(typ)}}}, "
                code += line[:-2] + "};\n"
        return code[:-1]

def generate_syntax_rules_string(info: SyntaxInfo, keywords: list[list[tuple[str, str]]]) -> str:
        code = f"static syntax_rules_t {info.file_type.upper()}_RULES = {{\"{info.string_chars}\", \"{info.short_comment}\", \"{info.long_comment_start}\", \"{info.long_comment_end}\", {{\n"
        for i, kw_list in enumerate(keywords):
                if i % 7 == 0:
                        code = code[:-1] + '\n' + " " * 8
                if kw_list:
                        code += f"{{{len(kw_list)}, FT_{info.abbreviation.upper()}_{'_' if i == 26 else ascii_uppercase[i]}}}, "
                else:
                        code += "{0, NULL}, "
        return code[:-1] + "\n}};"

def get_syntax_info() -> SyntaxInfo:
        return SyntaxInfo(
                file_type=input("Enter the name of the file type: ").replace('"', '\\"'),
                abbreviation=input("Enter the abbreviation for the file extension: ").replace('"', '\\"'),
                string_chars=input("Enter the characters that delimit a string: ").replace('"', '\\"'),
                short_comment=input("Enter the phrase that starts an inline comment: ").replace('"', '\\"'),
                long_comment_start=input("Enter the phrase that starts a long comment: ").replace('"', '\\"'),
                long_comment_end=input("Enter the phrase that ends a long comment: ").replace('"', '\\"'),
        )

def main():

        info = get_syntax_info()
        keywords = fill_keyword_list()
        keyword_list_declarations = generate_keyword_string(keywords, info.abbreviation)
        syntax_rules_declaration = generate_syntax_rules_string(info, keywords)
        
        print()
        print(keyword_list_declarations)
        print()
        print(syntax_rules_declaration)

if __name__ == "__main__":
        main()