using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.IO;
using System.Web;
using System.Text.RegularExpressions;

namespace BindingsGenerator
{
    public enum VisibilityLevel
    {
        Public,
        Protected,
        Private
    };

    public enum Virtualness
    {
        None,
        Virtual,
        Pure
    };

    /// <summary>
    /// "int foo" or "const char *str"
    /// </summary>
    public class Parameter
    {
        // int, or const char *, etc..
        public string type;
        // The name of the symbol.
        public string name;
        // The default value for this parameter in the function call.
        public string defaultValue;
        // The @param documentation associated with this parameter.
        public string comment;

        public bool IsAPointer()
        {
            return type.Trim().EndsWith("*");
        }

        public bool IsAReference()
        {
            // In terms of Tundra bindings, a reference to a shared ptr is interpreted as "not a reference"
            return type.Trim().EndsWith("&") && !type.Contains("Ptr");
        }

        /// <summary>
        /// If type "const float3 &" -> BasicType() returns "float3".
        /// If type "const float3 &" -> BasicType() returns "float3".
        /// If type "float3 * const" -> BasicType() returns "float3*".
        /// If type "const float3 * const" -> BasicType() returns "const float3*".
        /// </summary>
        /// <returns></returns>
        public string BasicType()
        {
            string t = type.Trim();
            if (t.EndsWith("&"))
            {
                t = t.Substring(0, t.Length - 1).Trim();
                if (t.StartsWith("const"))
                    t = t.Substring(5).Trim();
            }
            if (t.EndsWith("const"))
                t = t.Substring(0, t.Length - 5).Trim();
            return t;
        }

        public string BasicTypeRetainReference()
        {
            string t = type.Trim();
            if (t.StartsWith("const"))
                t = t.Substring(5).Trim();
            if (t.EndsWith("const"))
                t = t.Substring(0, t.Length - 5).Trim();
            return t;
        }

        /// <summary>
        /// Returns the basic type as a sanitated identifier.
        /// If type "const float3 &" -> BasicType() returns "float3".
        /// If type "const float3 &" -> BasicType() returns "float3".
        /// If type "float3 * const" -> BasicType() returns "float3_ptr".
        /// If type "const float3 * const" -> BasicType() returns "float3_ptr".
        /// </summary>
        /// <returns></returns>
        public string BasicTypeId()
        {
            string t = type.Trim();
            if (t.EndsWith("&"))
                t = t.Substring(0, t.Length - 1).Trim();
            if (t.StartsWith("const"))
                t = t.Substring(5).Trim();
            if (t.EndsWith("const"))
                t = t.Substring(0, t.Length - 5).Trim();
            if (t.EndsWith("*"))
            {
                t = t.Substring(0, t.Length - 1).Trim();

                if (t.EndsWith("const"))
                    t = t.Substring(0, t.Length - 5).Trim();

                t = t + "_ptr";
            }
            return t;
        }
    };

    public class Symbol
    {
        public string kind; // "file", "function", "class", "variable", "struct", "enumvalue", "typedef"
        /// <summary>
        /// If this is a function, specifies the return value.
        /// If this is a member variable, specifies the variable type.
        /// If this is a class or a struct, this field is empty.
        /// </summary>
        public string type;
        public VisibilityLevel visibilityLevel;
        public Virtualness virtualness;
        /// <summary>
        /// The name of the symbol this structure represents.
        /// </summary>
        public string name;
        public string id; // The string that was used as the key to add to the global symbol map.
        public string fullDefinition;
        public string path;
        public string filename;
        public string argList;
        /// <summary>
        /// Enum initializer value. Null for others
        /// </summary>
        public string value;

        /// <summary>
        /// If kind == "define", then this symbol is a #define macro, and this string contains the body of the macro.
        /// </summary>
        public string macroBody = "";
        /// <summary>
        /// Specifies under which name this symbol will be present in the class member index.
        /// If empty, this member will be hidden from the index.
        /// </summary>
        public string classMemberIndexTitle = "";
        public bool isStatic;
        public bool isMutable;
        public bool isExplicit;
        public bool isConst;

        /// <summary>
        /// The code file where this symbol is defined.
        /// </summary>
        public string sourceFilename;
        public int sourceFileStartLine;
        public int sourceFileEndLine;
        /// <summary>
        /// If this Symbol is "function", this contains a list of all the parameters to the function.
        /// </summary>
        public List<Parameter> parameters = new List<Parameter>();
        public List<string> notes = new List<string>();
        public List<string> todos = new List<string>();
        public List<string> bugs = new List<string>();
//        public Dictionary<string, Symbol> children = new Dictionary<string,Symbol>();
        public List<Symbol> children = new List<Symbol>();
        /// <summary>
        ///  The symbol this symbol is contained in.
        /// </summary>
        public Symbol parent;

        /// <summary>
        /// If this symbol is a class or a struct, this refers to the symbols that are the base classes of this class, or an empty list if no inheritance is used.
        /// \todo This has not been implemented yet. Doxygen has <base>BaseclassName</base> elements, implement this!
        /// </summary>
        public List<Symbol> baseClasses = new List<Symbol>();
        /// <summary>
        /// If non-null, this member specifies an overload of this function that is shown in the
        /// doc webpage instead of this symbol. (This is to make the docs look shorter).
        /// </summary>
        public Symbol similarOverload;
        /// <summary>
        /// This list specifies which other symbols use this symbol as the master doc page.
        /// </summary>
        public List<Symbol> otherOverloads = new List<Symbol>();
        // If kind == "file", this specifies all files this file includes.
        public List<string> includes = new List<string>();
//        public List<string> comments = new List<string>();

        /// <summary>
        /// If true, the generated documentation page should group the C++ syntax block with the other overloads of this function.
        /// </summary>
        public bool groupSyntax = false;

        public string briefDescription = "";
        public string detailedDescription = "";
        public string inbodyDescription = "";
        public string seeAlsoDocumentation = "";
        public string author = "";
        /// <summary>
        /// Specifies the documentation for the @return field.
        /// </summary>
        public string returnComment = "";

        /// <summary>
        /// If this Symbol is a class or a struct, returns the child symbol that is considered to be the ctor of this class.
        /// </summary>
        /// <returns></returns>
        public Symbol ClassCtor()
        {
            foreach (Symbol s in children)
                if (s.NameWithoutNamespace() == this.name)
                    if (s.similarOverload != null)
                        return s.similarOverload;
                    else
                        return s;

            return children.First();
        }

        public string BriefComment()
        {
            if (briefDescription.Length > 0)
                return briefDescription;
            else if (detailedDescription.Length > 0)
            {
                int comma = detailedDescription.IndexOf(".");
                if (comma == -1)
                    comma = detailedDescription.Length;
                return detailedDescription.Substring(0, comma);
            }
//            else if (returnComment.Length > 0)
//                return returnComment;
            else
                return "";
        }

        public string documentationCategory = "";

        public string Namespace()
        {
            int i = name.LastIndexOf("::");
            if (i != -1)
                return name.Substring(0, i);
            return "";
        }

        public string ArgStringWithoutTypes()
        {
            string s = "(";
            for (int i = 0; i < parameters.Count; ++i)
            {
                if (i > 0)
                    s += ",";
                s += parameters[i].name;
            }
            s += ")";
            return s;
        }

        public Symbol FindChildByName(string name)
        {
            foreach (Symbol s in children)
                if (s.name == name)
                    return s;
            return null;
        }

        public string NameWithoutNamespace()
        {
            string ns = Namespace();
            string nameWithoutNamespace = name.Substring(ns.Length);
            if (nameWithoutNamespace.StartsWith("::"))
                return nameWithoutNamespace.Substring(2);
            else
                return nameWithoutNamespace;
        }

        public bool IsArray()
        {
            return argList.StartsWith("[") && argList.EndsWith("]");
        }
        public int ArrayLength()
        {
            int res = 0;
            int.TryParse(argList.Substring(1, argList.Length - 2), out res);
            return res;
        }
        public bool IsConst()
        {
            if (isConst)
                return true;
            if (kind == "function" && argList.EndsWith("const"))
                return true;
            else if (kind == "variable" && type.Contains("const"))
                return true;
            else
                return false;
        }

        public Parameter FindParamByName(string paramName)
        {
            foreach (Parameter p in parameters)
                if (p.name == paramName)
                    return p;
            return null;
        }

        public string[] Comments()
        {
            List<string> c = new List<string>();
            if (briefDescription.Length > 0)
                c.Add(briefDescription);
            if (detailedDescription.Length > 0)
                c.Add(detailedDescription);
            if (inbodyDescription.Length > 0)
                c.Add(inbodyDescription);
            return c.ToArray();
        }

        public string Category()
        {
            return documentationCategory;
            /*
            if (category != "")
                return category;

            foreach (string s in Comments())
            {
                int endIdx;
                string categ = CodeStructure.FindStringInBetween(s, 0, "[Category:", "]", out endIdx); 
                if (endIdx != -1)
                {
                    string cat = categ.StripHtmlCharacters();
                    cat = cat.Replace("<br />", "").Replace("<br/>", "").StripHtmlCharacters();
                    if (cat.EndsWith("."))
                        cat = cat.Substring(0, cat.Length - 1).StripHtmlCharacters();
                    return cat;
                }
            }
            return "";*/
        }

        public string ScopeName()
        {
            string s = "";
            Symbol p = parent;
            while (p != null && (p.kind == "class" || p.kind == "struct"))
            {
                s = p.name + "::" + s;
                p = p.parent;
            }
            return s;
        }

        public string ScopeNameWithoutNamespace()
        {
            string s = "";
            Symbol p = parent;
            while (p != null && (p.kind == "class" || p.kind == "struct"))
            {
                s = p.NameWithoutNamespace() + "::" + s;
                p = p.parent;
            }
            return s;
        }

        public string FullQualifiedSymbolName()
        {
            string s = type + " ";
            s += ScopeName();
            s += name;
            s += argList;
            return s;
        }

        public string FullQualifiedSymbolNameWithoutNamespace()
        {
            if (kind == "define")
            {
                string s = "#define " + NameWithoutNamespace() + argList;
                return s;
            }
            else
            {
                string s = type + " ";
                s += ScopeNameWithoutNamespace();
                s += NameWithoutNamespace();
                s += argList;
                return s;
            }
        }

        public const string pageSuffix = "php";

        static string EscapeFilename(string s)
        {
            s = s.Replace(".html", "_")
                .Replace(".", "_")
                .Replace("*", "__mul__")
                .Replace("/", "__div__")
                .Replace("+", "__plus__")
                .Replace("-", "__minus__")
                .Replace("::", "__scope__")
                .Replace(":", "__dc__")
                .Replace("<=", "__leq__")
                .Replace("<", "__lt__")
                .Replace(">=", "__geq__")
                .Replace(">", "__gt__")
                .Replace("==", "__eqa__")
                .Replace("=", "__eq__")
                .Trim();
            return s;
        }

        public string ClassIndexDocFilename()
        {
            return "index_" + EscapeFilename(name) + "." + pageSuffix;
        }

        public string MemberDocumentationFilename()
        {
            return MemberDocumentationFilenameWithoutFileSuffix() + "." + pageSuffix;
        }

        public string MemberDocumentationFilenameWithoutFileSuffix()
        {
            if (kind == "class" || kind == "struct")
                return EscapeFilename(name) + "_summary";
            if (similarOverload != null)
                return similarOverload.MemberDocumentationFilenameWithoutFileSuffix();
            if (parent != null)
                return EscapeFilename(parent.name) + "_" + EscapeFilename(name);
            else
                return EscapeFilename(name);
        }

        public static bool IsPODType(string type)
        {
            return type == "bool" || type == "int" || type == "unsigned" || type == "unsigned short" || type == "u32" || type == "u16" || type == "uint" || type == "double" || type == "float" || type == "AttributeChange::Type" || type == "ClientLoginState" || type == "entity_id_t" || type == "component_id_t"; ///\todo Add more basic types here.
        }

        public static bool IsNumberType(string type)
        {
            return type == "int" || type == "unsigned" || type == "unsigned short" || type == "u32" || type == "u16" || type == "uint" || type == "double" || type == "float" || type == "AttributeChange::Type" || type == "ClientLoginState" || type == "entity_id_t" || type == "component_id_t";
        }
    };

    public class VariableListEntry
    {
        public string item;
        public string value;
    };

    public class CodeFile
    {
        public string filename;
        public List<string> lines = new List<string>();
    };

    public class CodeStructure
    {
        public Dictionary<string, CodeFile> codeFiles = new Dictionary<string, CodeFile>();
        public Dictionary<string, Symbol> symbols = new Dictionary<string, Symbol>();
        public Dictionary<string, Symbol> symbolsByName = new Dictionary<string,Symbol>();
        public Dictionary<string, Symbol> enumsByName = new Dictionary<string, Symbol>();
        public SortedSet<string> documentationFiles = new SortedSet<string>();

        public List<VariableListEntry> todos = new List<VariableListEntry>();
        public List<VariableListEntry> bugs = new List<VariableListEntry>();

        /// <summary>
        /// Goes through all symbols recursively and groups similar functions together (sets the similarOverload attribute).
        /// </summary>
        void GroupSimilarOverloads()
        {
            foreach(Symbol s in symbols.Values)
                if (s.kind == "class" || s.kind == "struct")
                    GroupSimilarOverloads(s);
        }

        void GroupSimilarOverloads(Symbol s)
        {
            for(int i = 0; i < s.children.Count; ++i)
                for(int j = i+1; j < s.children.Count; ++j)
                {
                    Symbol master = s.children[i];
                    Symbol child = s.children[j];
                    if (master.similarOverload == null && child.similarOverload == null && child.otherOverloads.Count == 0)
                    {
                        if (master.name == child.name)
                        {
                            if (child.Comments().Length == 0 || child.groupSyntax)
                            {
                                child.similarOverload = master;
                                master.otherOverloads.Add(child);
                            }
                            else
                                break;
                        }
                    }
                }
        }       

        // From http://stackoverflow.com/questions/475293/change-the-node-names-in-an-xml-file-using-c-sharp .
        static XmlElement RenameElement(XmlNode parent, XmlNode child, string newName)
        {
            // get existing 'Content' node
//            XmlNode contentNode = parent.SelectSingleNode("Content");

            // create new (renamed) Content node
            XmlElement newNode = parent.OwnerDocument.CreateElement(newName);

            // [if needed] copy existing Content children
            newNode.InnerXml = child.InnerXml;
            foreach(XmlAttribute attr in child.Attributes)
                newNode.SetAttribute(attr.Name, attr.Value);

            // replace existing Content node with newly renamed Content node
            parent.InsertBefore(newNode, child);
            parent.RemoveChild(child);
            return newNode;
        }

        static void RenameAttribute(XmlElement elem, string oldAttrName, string newAttrName)
        {
            elem.SetAttribute(newAttrName, elem.GetAttribute(oldAttrName));
            elem.RemoveAttribute(oldAttrName);
        }

        static void ProcessNodes(XmlElement e)
        {
            foreach (XmlElement c in e.ChildNodes.OfType<XmlElement>())
            {
                XmlElement d = null;
                if (c.Name == "image")
                {
                    RenameAttribute(c, "name", "src");
                    d = RenameElement(e, c, "img");
                }
                else if (c.Name == "para")
                    d = RenameElement(e, c, "p");
                else if (c.Name == "ulink")
                {
                    RenameAttribute(c, "url", "href");
                    d = RenameElement(e, c, "a");
                }
                else if (c.Name == "itemizedlist")
                    d = RenameElement(e, c, "ul");
                else if (c.Name == "listitem")
                    d = RenameElement(e, c, "li");

                if (d == null)
                    ProcessNodes(c);
                else // We processed a child. foreach borks. re-do.
                {
                    ProcessNodes(e);
                    return;
                }
            }
        }

        static private string GetXmlElementChildNodeValue(XmlElement element, string childNode, bool asText = false)
        {
            foreach (XmlElement e in element.ChildNodes)
            {
                if (e.Name == childNode)
                {
                    ProcessNodes(e);
                    string xml;
                    if (asText)
                        xml = e.InnerText;
                    else
                        xml = e.InnerXml;
                    xml = xml.Trim();
                    if (xml.StartsWith("<p>") && xml.EndsWith("</p>"))
                        xml = xml.Substring(3, xml.Length - 7).Trim();
                    if (xml == "<p />")
                        return "";
                    return xml;
                    /*
                    if (e.InnerXml.Contains("umlaut"))
                        e.InnerXml = e.InnerXml.Replace("<umlaut char=\"o\" />", "ö").Replace("<umlaut char=\"a\" />", "ä");
                    return e.InnerText.Replace("ö", "&ouml;").Replace("ä", "&ouml;");
                     */
                }
//                    return e
//                    return e.InnerText;
            }
            return "";
        }

        static List<XmlElement> GetChildElementsByName(XmlElement node, string elementName)
        {
            List<XmlElement> elements = new List<XmlElement>();
            foreach (XmlNode n in node.ChildNodes)
            {
                XmlElement e = n as XmlElement;
                if (e == null)
                    continue;
                if (e.Name == elementName)
                    elements.Add(e);
            }
            return elements;
        }

        static private VisibilityLevel ParseVisibilityLevel(string s)
        {
            if (s == null || s == "")
                return VisibilityLevel.Public;
            if (s == "protected")
                return VisibilityLevel.Protected;
            if (s == "private")
                return VisibilityLevel.Private;
            return VisibilityLevel.Public;
        }

        static private Virtualness ParseVirtualness(string s)
        {
            if (s == null || s == "")
                return Virtualness.None;
            if (s == "virtual")
                return Virtualness.Virtual;
            if (s == "pure")
                return Virtualness.Pure;
            return Virtualness.None;
        }

        private string CutDocGeneratorCommentDirective(string s, int startIdx, int endIdx)
        {
            s = s.Remove(startIdx, endIdx);
            if (s.Length > startIdx && s[startIdx] == '.') // Eat the trailing dot, if there was one.
                s = s.Remove(startIdx, 1);
            if (s.Length > startIdx+1 && s[startIdx+1] == '.') // Eat the trailing dot, if there was one.
                s = s.Remove(startIdx+1, 1);
            s = s.Trim();
            return s;
        }

        /// <summary>
        /// Goes through the given symbol and find all DocGenerator-style directives present in the comments
        /// and applies them. These directives are of form [foo].
        /// </summary>
        private void ProcessDocGeneratorCommentDirectives(Symbol s)
        {
//            string[] comments = s.Comments();
            string[] comments = new string[3] { s.briefDescription, s.detailedDescription, s.inbodyDescription };
            for (int i = 0; i < comments.Length; ++i)
            {
                comments[i] = comments[i].Trim();
                if (comments[i].EndsWith("<br/>"))
                    comments[i] = comments[i].Substring(0, comments[i].Length - 5);
                if (comments[i].EndsWith("<br />"))
                    comments[i] = comments[i].Substring(0, comments[i].Length - 6);
                comments[i] = comments[i].Trim();

                int endIdx;
                int startIndex = 0;
                while (startIndex < comments[i].Length)
                {
                    string directive = FindStringInBetween(comments[i], startIndex, "[", "]", out endIdx);
                    if (endIdx == -1)
                        break;

                    int directiveStartIndex = endIdx - directive.Length - 1;
                    directive = directive.Trim();
                    string[] st = directive.Split(':');
                    string directiveParam = "";
                    if (st.Length == 2)
                        directiveParam = st[1].Trim();

                    if (directive.ToLower().StartsWith("similaroverload") && directiveParam.Length > 0)
                    {
                        Symbol similarOverloadSymbol = s.parent.FindChildByName(directiveParam);
                        if (similarOverloadSymbol != null)
                        {
                            if (s.similarOverload == null)
                            {
                                s.similarOverload = similarOverloadSymbol;
                                similarOverloadSymbol.otherOverloads.Add(s);
                            }
                        }
                        else
                        {
//                            Console.WriteLine("Can't find similarOverload " + directiveParam + " for member " + s.FullQualifiedSymbolName());
                        }
                        comments[i] = CutDocGeneratorCommentDirective(comments[i], directiveStartIndex - 1, endIdx + 1 - directiveStartIndex);
                        startIndex = 0;

                        // Don't update startIndex since we deleted the "[]" block.
                    }
                    else if (directive.ToLower().StartsWith("indextitle") && directiveParam.Length > 0)
                    {
                        s.classMemberIndexTitle = directiveParam;
                        comments[i] = CutDocGeneratorCommentDirective(comments[i], directiveStartIndex - 1, endIdx + 1 - directiveStartIndex);
                        startIndex = 0;
                    }
                    else if (directive.ToLower().StartsWith("hideindex"))
                    {
                        s.classMemberIndexTitle = "";
                        comments[i] = CutDocGeneratorCommentDirective(comments[i], directiveStartIndex - 1, endIdx + 1 - directiveStartIndex);
                        startIndex = 0;
                    }
                    else if (directive.ToLower().StartsWith("category"))
                    {
                        s.documentationCategory = directiveParam;
                        comments[i] = CutDocGeneratorCommentDirective(comments[i], directiveStartIndex - 1, endIdx + 1 - directiveStartIndex);
                        startIndex = 0;
                    }
                    else if (directive.ToLower().StartsWith("groupsyntax"))
                    {
                        s.groupSyntax = true;
                        comments[i] = CutDocGeneratorCommentDirective(comments[i], directiveStartIndex - 1, endIdx + 1 - directiveStartIndex);
                        startIndex = 0;
                    }
                    else
                    {
                        startIndex = endIdx;
                    }
                }
            }
            s.briefDescription = comments[0];
            s.detailedDescription = comments[1];
            s.inbodyDescription = comments[2];
        }

        /// <summary>
        /// Outputs endIdx = -1 if not found.
        /// </summary>
        /// <param name="dataString"></param>
        /// <param name="startIdx"></param>
        /// <param name="start"></param>
        /// <param name="end"></param>
        /// <param name="endIdx"></param>
        /// <returns></returns>
        public static string FindStringInBetween(string dataString, int startIdx, string start, string end, out int endIdx)
        {
            endIdx = -1;
            int patternStart = dataString.IndexOf(start, startIdx);
            if (patternStart == -1)
                return "";
            patternStart += start.Length;
            int patternEnd = dataString.IndexOf(end, patternStart);
            if (patternEnd == -1)
                return "";
            string text = dataString.Substring(patternStart, patternEnd - patternStart);
            endIdx = patternEnd + end.Length;
            return text;
        }

        public void ParseSectionDefElement(Symbol parent, XmlElement e)
        {
            foreach(XmlElement child in e.ChildNodes)
            {
                if (child.Name == "memberdef")
                {
                    Symbol member = new Symbol();
                    member.parent = parent;
                    member.id = child.GetAttribute("id");
                    member.kind = child.GetAttribute("kind");
                    member.visibilityLevel = ParseVisibilityLevel(child.GetAttribute("prot"));
                    member.isStatic = (child.GetAttribute("static") == "yes");
                    member.isConst = (child.GetAttribute("const") == "yes");
                    member.isMutable = (child.GetAttribute("mutable") == "yes");
                    member.isExplicit = (child.GetAttribute("explicit") == "yes");
                    member.virtualness = ParseVirtualness(child.GetAttribute("virt"));
                    member.type = GetXmlElementChildNodeValue(child, "type", true);
                    member.fullDefinition = GetXmlElementChildNodeValue(child, "definition");
                    member.argList = GetXmlElementChildNodeValue(child, "argsstring");

                    // Sanitate return type of certain MathGeoLib constructs
                    if (member.kind == "function")
                    {
                        member.type = member.type.Replace("CONST_WIN32", "");
                        member.type = member.type.Replace("MUST_USE_RESULT", "");
                    }

                    member.classMemberIndexTitle = member.name = GetXmlElementChildNodeValue(child, "name");
                    if (member.name.StartsWith("@"))
                        continue; // Doxygen creates items with names starting with '@' at least for unnamed unions, ignore those altogether.
                    symbols[member.id] = member;
                    //symbolsByName[member.name] = member;
                    parent.children.Add(member);

                    // Function parameters.
                    foreach(XmlElement param in child.ChildNodes.OfType<XmlElement>())
                        if (param.Name == "param")
                        {
                            Parameter p = new Parameter();
                            if (member.kind == "define") // This is a #define macro.
                            {
                                p.type = "";
                                p.name = GetXmlElementChildNodeValue(param, "defname");
                            }
                            else // This is a real function
                            {
                                p.type = GetXmlElementChildNodeValue(param, "type", true);
                                p.name = GetXmlElementChildNodeValue(param, "declname");
                                p.defaultValue = GetXmlElementChildNodeValue(param, "defval", true);
                            }
                            member.parameters.Add(p);
                        }

                    // Enum special handling
                    if (member.kind == "enum")
                    {
                        int lastEnumInitializer = -1;

                        foreach (XmlElement value in child.ChildNodes.OfType<XmlElement>())
                        {
                            if (value.Name == "enumvalue")
                            {
                                Symbol valueSymbol = new Symbol();
                                valueSymbol.parent = member;
                                valueSymbol.id = value.GetAttribute("id");
                                valueSymbol.name = GetXmlElementChildNodeValue(value, "name", true);
                                valueSymbol.kind = "enumvalue";
                                valueSymbol.value = GetXmlElementChildNodeValue(value, "initializer", true).Replace("= ", "");
                                if (valueSymbol.value.Length == 0)
                                {
                                    ++lastEnumInitializer;
                                    valueSymbol.value = lastEnumInitializer.ToString();
                                }
                                else
                                    Int32.TryParse(valueSymbol.value, out lastEnumInitializer);
                                member.children.Add(valueSymbol);
                            }
                        }
                        enumsByName[member.name] = member;
                    }

                    // If this is a #define macro, get the macro body code.
                    if (member.kind == "define")
                    {
                        member.macroBody = GetXmlElementChildNodeValue(child, "initializer");
                        // Create the argList from scratch, because it is not present in the xml in same form than for functions.
                        member.argList = member.ArgStringWithoutTypes();
                    }

                    // Function parameter comments.
                    List<XmlElement> parameters = child.AllGrandChildElementsOfTypeAndAttribute("parameterlist", "kind", "param");
                    if (parameters != null && parameters.Count > 0)
                    {
                        foreach (XmlElement paramNode in parameters.OfType<XmlElement>())
                        {
                            foreach (XmlElement param in paramNode.ChildNodes.OfType<XmlElement>())
                            {
                                if (param.Name == "parameteritem")
                                {
                                    string paramName = param.FirstChildElementOfType("parameternamelist").FirstChildElementOfType("parametername").InnerText.Trim();
                                    Parameter p = member.FindParamByName(paramName);
                                    if (p != null)
                                        p.comment = param.FirstChildElementOfType("parameterdescription").InnerText;
                                }
                            }
                            // Remove the parameterlist from the detailed description node so that it won't appear in the 'detailedDescription' documentation string.
                            // The detailed description is created manually.
                            paramNode.ParentNode.RemoveChild(paramNode);
                        }
                    }

                    // TODOs ^ BUGs 
                    List<XmlElement> xrefsects = child.AllGrandChildElementsOfType("xrefsect");
                    if (xrefsects != null)
                    {
                        foreach (XmlElement elem in xrefsects)
                        {
                            if (GetXmlElementChildNodeValue(elem, "xreftitle") == "Todo")
                                member.todos.Add(GetXmlElementChildNodeValue(elem, "xrefdescription"));
                            if (GetXmlElementChildNodeValue(elem, "xreftitle") == "Bug")
                                member.bugs.Add(GetXmlElementChildNodeValue(elem, "xrefdescription"));
                            elem.ParentNode.RemoveChild(elem);
                        }
                    }

                    // @notes.
                    List<XmlElement> notesects = child.AllGrandChildElementsOfTypeAndAttribute("simplesect", "kind", "note");
                    if (notesects != null)
                    {
                        foreach (XmlElement elem in notesects)
                        {
                            member.notes.Add(elem.InnerText);
                            elem.ParentNode.RemoveChild(elem);
                        }
                    }

                    // Function return value.
                    XmlElement retVal = child.FirstGrandChildElementOfTypeAndAttribute("simplesect", "kind", "return");
                    if (retVal != null)
                    {
                        member.returnComment = retVal.InnerText;
                        retVal.ParentNode.RemoveChild(retVal);
                    }

                    // The "see also" section.
                    XmlElement see = child.FirstGrandChildElementOfTypeAndAttribute("simplesect", "kind", "see");
                    if (see != null)
                    {
                        member.seeAlsoDocumentation = see.InnerXml;
                        see.ParentNode.RemoveChild(see);
                    }

                    member.briefDescription = GetXmlElementChildNodeValue(child, "briefdescription").Trim();
                    member.inbodyDescription = GetXmlElementChildNodeValue(child, "inbodydescription").Trim();
                    member.detailedDescription = GetXmlElementChildNodeValue(child, "detaileddescription").Trim();
                    XmlElement loc = child.FirstChildElementOfType("location");
                    if (loc != null)
                    {
                        member.sourceFilename = loc.GetAttribute("bodyfile");
                        int.TryParse(loc.GetAttribute("bodystart"), out member.sourceFileStartLine);
                        int.TryParse(loc.GetAttribute("bodyend"), out member.sourceFileEndLine);
                        if (member.sourceFileEndLine == -1)
                            member.sourceFileEndLine = member.sourceFileStartLine;
                    }
                    ProcessDocGeneratorCommentDirectives(member);

                    ///\todo Add location.
                }
            }            
        }

        public void ParseTodoBugList(XmlElement e, out List<VariableListEntry> list)
        {
            List<VariableListEntry> l = new List<VariableListEntry>();
            e = e.FirstChildElementOfType("detaileddescription").FirstChildElementOfType("para").FirstChildElementOfType("variablelist");
            VariableListEntry entry = null;
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
            {
                if (child.Name == "varlistentry")
                {
                    entry = new VariableListEntry();
                    entry.item = child.InnerXml;
                    l.Add(entry);
                }
                if (child.Name == "listitem")
                    entry.value = child.InnerXml;
            }
            list = l;
        }

        public void ParseCodeFileList(XmlElement e)
        {
            CodeFile codeFile = new CodeFile();
            codeFile.filename = e.FirstChildElementOfType("compoundname").InnerText;
            XmlElement l = e.FirstGrandChildElementOfType("programlisting");
            if (l == null)
                return;
            int prevLine = 0;
            foreach (XmlElement c in l.ChildNodes.OfType<XmlElement>())
                if (c.Name == "codeline")
                {
                    int curLine = int.Parse(c.GetAttribute("lineno"));
                    for (int i = 0; i < curLine - 1 - prevLine; ++i)
                        codeFile.lines.Add("");
                    codeFile.lines.Add(c.InnerXml);
                    prevLine = curLine;
                }
            codeFiles[e.FirstChildElementOfType("location").GetAttribute("file")] = codeFile;
        }

        public void ParsePageCompoundDefElement(XmlElement e)
        {
            if (e.GetAttribute("id") == "bug")
                ParseTodoBugList(e, out bugs);
            else if (e.GetAttribute("id") == "todo")
                ParseTodoBugList(e, out todos);
        }
        
        public void ParseCompoundDefElement(XmlElement e)
        {
            Symbol s = new Symbol();
            s.id = e.GetAttribute("id");
            s.kind = e.GetAttribute("kind");
            s.visibilityLevel = ParseVisibilityLevel(e.GetAttribute("prot"));
            s.classMemberIndexTitle = s.name = GetXmlElementChildNodeValue(e, "compoundname");

            // The "author" section.
            XmlElement author = e.FirstGrandChildElementOfTypeAndAttribute("simplesect", "kind", "author");
            if (author != null)
            {
                s.author = author.InnerXml;
                author.ParentNode.RemoveChild(author);
            }

            s.briefDescription = GetXmlElementChildNodeValue(e, "briefdescription");
            s.inbodyDescription = GetXmlElementChildNodeValue(e, "inbodydescription");
            s.detailedDescription = GetXmlElementChildNodeValue(e, "detaileddescription");
            symbols[s.id] = s;
            symbolsByName[s.name] = s;

            ProcessDocGeneratorCommentDirectives(s);

            foreach(XmlElement child in e.ChildNodes)
            {
                if (child.Name == "sectiondef")
                    ParseSectionDefElement(s, child);
            }            
            // Also has the following members:
            // <collaborationgraph>
            // <location>
            // <listofallmembers>
            // <includes>
        }

        public void LoadSymbolsFromFile(string filename)
        {
            TextReader t = new StreamReader(filename);
            string dataString = t.ReadToEnd();
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(dataString);

            XmlElement root = doc.DocumentElement;
            foreach (XmlNode node in root.ChildNodes)
            {
                XmlElement e = node as XmlElement;
                if (e == null)
                    continue;
                if (e.Name == "compounddef")
                {
                    if (e.GetAttribute("kind") == "page")
                        ParsePageCompoundDefElement(e);
                    else
                    {
                        if (e.GetAttribute("kind") == "file")
                            ParseCodeFileList(e);
                        ParseCompoundDefElement(e);
                    }
                }
//                if (e.Name == "compound" && (e.GetAttribute("kind") == "class" || e.GetAttribute("kind") == "struct"))
//                    ParseClassCompound(symbols, e);
            }
            foreach (XmlNode node in root.ChildNodes)
            {
                XmlElement e = node as XmlElement;
                if (e == null)
                    continue;
//                if (e.Name == "compound" && e.GetAttribute("kind") == "file")
 //                   ParseFileCompound(symbols, e);
            }
        }

        public void LoadSymbolsFromDirectory(string directory, bool recursive)
        {
            string[] docFiles = Directory.GetFiles(directory, "*.xml", recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly);
            foreach (string filename in docFiles)
                LoadSymbolsFromFile(filename);
            GroupSimilarOverloads();
        }
    }

    static public class XmlElementExtensions
    {
        public static XmlElement FirstChildElementOfType(this XmlElement e, string type)
        {
            foreach (XmlElement child in e.ChildNodes)
                if (child.Name == type)
                    return child;
            return null;
        }

        public static XmlElement FirstChildElementOfTypeAndAttribute(this XmlElement e, string type, string attr, string value)
        {
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
                if (child.Name == type && child.GetAttribute(attr) == value)
                    return child;
            return null;
        }

        public static List<XmlElement> AllChildElementsOfType(this XmlElement e, string type)
        {
            List<XmlElement> l = new List<XmlElement>();
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
                if (child.Name == type)
                    l.Add(child);
            return l;
        }

        public static List<XmlElement> AllChildElementsOfTypeAndAttribute(this XmlElement e, string type, string attr, string value)
        {
            List<XmlElement> l = new List<XmlElement>();
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
                if (child.Name == type && child.GetAttribute(attr) == value)
                    l.Add(child);
            return l;
        }

        public static XmlElement FirstGrandChildElementOfType(this XmlElement e, string type)
        {
            XmlElement t = FirstChildElementOfType(e, type);
            if (t != null)
                return t;
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
            {
                t = FirstGrandChildElementOfType(child, type);
                if (t != null)
                    return t;
            }
            return null;
        }

        public static XmlElement FirstGrandChildElementOfTypeAndAttribute(this XmlElement e, string type, string attr, string value)
        {
            XmlElement t = FirstChildElementOfTypeAndAttribute(e, type, attr, value);
            if (t != null)
                return t;
            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
            {
                t = FirstGrandChildElementOfTypeAndAttribute(child, type, attr, value);
                if (t != null)
                    return t;
            }
            return null;
        }

        public static List<XmlElement> AllGrandChildElementsOfType(this XmlElement e, string type)
        {
            List<XmlElement> l = new List<XmlElement>();
            List<XmlElement> l2 = AllChildElementsOfType(e, type);
            if (l2.Count > 0)
                return l2;

            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
            {
                l2 = AllGrandChildElementsOfType(child, type);
                if (l2 != null)
                    l.AddRange(l2);
            }
            return l;
        }

        public static List<XmlElement> AllGrandChildElementsOfTypeAndAttribute(this XmlElement e, string type, string attr, string value)
        {
            List<XmlElement> l = new List<XmlElement>();
            List<XmlElement> l2 = AllChildElementsOfTypeAndAttribute(e, type, attr, value);
            if (l2.Count > 0)
                return l2;

            foreach (XmlElement child in e.ChildNodes.OfType<XmlElement>())
            {
                l2 = AllGrandChildElementsOfTypeAndAttribute(child, type, attr, value);
                l.AddRange(l2);
            }
            return l;
        }
   }

    static public class StringExtensions
    {
        public static string StripHtmlCharacters(this string str)
        {            
            return System.Net.WebUtility.HtmlDecode(str.Replace("&nbsp;", " ").Replace("&#160;", " ").Replace("&amp;", "&")).Trim();
        }
        public static string StripHtmlLinks(this string str)
        {
            for(;;)
            { ///\todo regex.
                int firstA = str.IndexOf("<a");
                int firstCloseA = str.IndexOf("</a");
                if (firstA == -1) firstA = str.Length + 1;
                if (firstCloseA == -1) firstCloseA = str.Length + 1;
                int start = Math.Min(firstA, firstCloseA);
                if (start == str.Length + 1)
                    return str;
                int end = str.IndexOf(">", start);
                if (end == -1)
                    return str;
                string left = str.Substring(0, start);
                string right = str.Substring(end + 1);
                str = left + right;
            }
        }
    }

}
