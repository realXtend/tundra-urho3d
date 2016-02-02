using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace BindingsGenerator
{
    struct Overload
    {
        public string functionName;
        public Symbol function;
        public List<Parameter> parameters;
    }

    class Program
    {
        static HashSet<string> classNames = new HashSet<string>();

        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("First cmdline parameter should be the absolute path to the root where doxygen generated the documentation XML files.");
                Console.WriteLine("Second cmdline parameter should be the output directory.");
                return;
            }

            CodeStructure s = new CodeStructure();
            s.LoadSymbolsFromDirectory(args[0], true);

            string outputDirectory = args[1];

            foreach (Symbol classSymbol in s.symbolsByName.Values)
            {
                if (classSymbol.kind == "class")
                    classNames.Add(classSymbol.name);
            }

            foreach (Symbol classSymbol in s.symbolsByName.Values)
            {
                if (classNames.Contains(classSymbol.name))
                    GenerateClassBindings(classSymbol, outputDirectory);
            }
        }

        static void GenerateClassBindings(Symbol classSymbol, string outputDirectory)
        {
            Console.WriteLine("Generating bindings for " + classSymbol.name);

            TextWriter tw = new StreamWriter(outputDirectory + "/" + classSymbol.name + "Bindings.cpp");
            tw.WriteLine("#include \"BindingsHelpers.h\"");
            tw.WriteLine("#include \"" + classSymbol.name + ".h\"");
            tw.WriteLine("");
            tw.WriteLine("namespace Bindings");
            tw.WriteLine("{");

            // Find dependency classes and refer to them
            // Includes
            HashSet<string> dependencies = FindDependencies(classSymbol);
            foreach (string s in dependencies)
                tw.WriteLine("#include \"" + s + ".h\"");
            tw.WriteLine("");

            // Externs for the type identifiers
            foreach (string s in dependencies)
                tw.WriteLine("extern const char* " + ClassIdentifier(s) + ";");
            tw.WriteLine("");

            // Own type identifier
            tw.WriteLine("const char* " + ClassIdentifier(classSymbol.name) + " = \"" + classSymbol.name + "\";");
            tw.WriteLine("");

            // \todo Handle refcounted class destruction (wrap in a smart ptr)
            Dictionary<string, List<Overload> > overloads = new Dictionary<string, List<Overload> >();
            Dictionary<string, List<Overload>> staticOverloads = new Dictionary<string, List<Overload>>();
            GenerateDestructor(classSymbol, tw);
            GeneratePropertyAccessors(classSymbol, tw);
            GenerateMemberFunctions(classSymbol, tw, overloads, false);
            GenerateFunctionSelectors(classSymbol, tw, overloads);
            GenerateMemberFunctions(classSymbol, tw, staticOverloads, true);
            GenerateFunctionSelectors(classSymbol, tw, staticOverloads);
            GenerateFunctionList(classSymbol, tw, overloads, false);
            GenerateFunctionList(classSymbol, tw, staticOverloads, true);
       
            // \todo Create bindings for static functions
            // \todo Create code to instantiate the JS constructor + prototype

            tw.WriteLine("}");
            tw.Close();
        }

        static string GenerateGetFromStack(Symbol classSymbol, int stackIndex, string varName, bool nullCheck = false)
        {
            string typeName = classSymbol.type;
            if (typeName == null || typeName.Length == 0)
                typeName = classSymbol.name;

            return GenerateGetFromStack(typeName, stackIndex, varName, nullCheck);
        }

        static string GenerateGetFromStack(string typeName, int stackIndex, string varName, bool nullCheck = false)
        {
            if (!Symbol.IsPODType(typeName))
            {
                if (!nullCheck)
                    return typeName + "* " + varName + " = GetObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + ");";
                else
                    return typeName + "* " + varName + " = GetObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + "); " +
                            "if (!" + varName + ") duk_error(ctx, DUK_ERR_REFERENCE_ERROR, \"Null or invalid object argument\");";
            }
            else if (Symbol.IsNumberType(typeName))
                return typeName + " " + varName + " = duk_require_number(ctx, " + stackIndex + ");";
            else if (typeName == "bool")
                return typeName + " " + varName + " = duk_require_bool(ctx, " + stackIndex + ");";
            else
                throw new System.Exception("Unsupported type " + typeName + " for GenerateGetVariable()!");
        }

        static string GeneratePushToStack(Symbol classSymbol, string source)
        {
            string typeName = classSymbol.type;
            if (typeName == null || typeName.Length == 0)
                typeName = classSymbol.name;

            return GeneratePushToStack(typeName, source);
        }

        static string GeneratePushToStack(string typeName, string source)
        {
            if (Symbol.IsNumberType(typeName))
                return "duk_push_number(ctx, " + source + ");";
            else if (typeName == "bool")
                return "duk_push_boolean(ctx, " + source + ");";
            else
            {
                typeName = SanitateTypeName(typeName);
                return "duk_push_object(ctx); " +
                       "SetObject(ctx, -1, new " + typeName + "(" + source + "), " + ClassIdentifier(typeName) + "); " +
                       "duk_push_c_function(ctx, " + typeName + "_Dtor, 1); " +
                       "duk_set_finalizer(ctx, -2); " + 
                       "duk_get_global_string(ctx, " + ClassIdentifier(typeName) + "); " +
                       "duk_get_prop_string(ctx, -1, \"prototype\"); " +
                       "duk_set_prototype(ctx, -3); " +
                       "duk_pop(ctx);";
            }           
        }

        static string GeneratePushConstructorResultToStack(string typeName, string source)
        {
            return "duk_push_this(ctx); " +
                   "SetObject(ctx, -1, " + source + ", " + ClassIdentifier(typeName) + "); " +
                   "duk_push_c_function(ctx, " + typeName + "_Dtor, 1); " +
                   "duk_set_finalizer(ctx, -2);";
        }

        static string GenerateGetThis(Symbol classSymbol, string varName = "thisObj")
        {
            string typeName = classSymbol.type;
            if (typeName == null || typeName.Length == 0)
                typeName = classSymbol.name;

            return typeName + "* " + varName + " = GetThisObject<" + typeName + ">(ctx, " + ClassIdentifier(typeName) + "); if (!" + varName + ") duk_error(ctx, DUK_ERR_REFERENCE_ERROR, \"Null this pointer\");";
        }

        static string GenerateArgCheck(Parameter p, int stackIndex)
        {
            string typeName = p.BasicType();

            if (!Symbol.IsPODType(typeName))
                return "GetObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + ")";
            else if (Symbol.IsNumberType(typeName))
                return "duk_is_number(ctx, " + stackIndex + ")";
            else if (typeName == "bool")
                return "duk_is_boolean(ctx, " + stackIndex + ")";

            throw new System.Exception("Unsupported type " + typeName + " for GenerateArgCheck()!");
        }

        static void GenerateDestructor(Symbol classSymbol, TextWriter tw)
        {
            tw.WriteLine("duk_ret_t " + classSymbol.name + "_Dtor" + DukSignature());
            tw.WriteLine("{");
            tw.WriteLine(Indent(1) + GenerateGetFromStack(classSymbol, 0, "obj"));
            tw.WriteLine(Indent(1) + "if (obj)");
            tw.WriteLine(Indent(1) + "{");
            tw.WriteLine(Indent(2) + "delete obj;");
            tw.WriteLine(Indent(2) + "SetObject(ctx, 0, 0, " + ClassIdentifier(classSymbol.name) + ");");
            tw.WriteLine(Indent(1) + "}");
            tw.WriteLine(Indent(1) + "return 0;");
            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GeneratePropertyAccessors(Symbol classSymbol, TextWriter tw)
        {
            foreach (Symbol child in classSymbol.children)
            {
                // \todo Handle non-POD accessors
                if (child.kind == "variable" && !child.isStatic && IsScriptable(child) && child.visibilityLevel == VisibilityLevel.Public && Symbol.IsPODType(child.type))
                {
                    // Set accessor
                    if (!child.IsConst())
                    {
                        tw.WriteLine("duk_ret_t " + classSymbol.name + "_Set_" + child.name + DukSignature());
                        tw.WriteLine("{");
                        tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                        tw.WriteLine(Indent(1) + GenerateGetFromStack(child, 0, child.name));
                        tw.WriteLine(Indent(1) + "thisObj->" + child.name + " = " + child.name);
                        tw.WriteLine(Indent(1) + "return 0;");
                        tw.WriteLine("}");
                        tw.WriteLine("");
                    }

                    // Get accessor
                    {
                        tw.WriteLine("duk_ret_t " + classSymbol.name + "_Get_" + child.name + DukSignature());
                        tw.WriteLine("{");
                        tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                        tw.WriteLine(Indent(1) + GeneratePushToStack(child, "thisObj->" + child.name));
                        tw.WriteLine(Indent(1) + "return 1;");
                        tw.WriteLine("}");
                        tw.WriteLine("");                                                                                      
                    }
                }
            }
        }

        static void GenerateMemberFunctions(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads, bool generateStatic)
        {
            foreach (Symbol child in classSymbol.children)
            { 
                if (child.isStatic == generateStatic && child.kind == "function" && !child.name.Contains("operator"))
                {
                    if (!IsScriptable(child))
                        continue;

                    bool isClassCtor = !child.isStatic && (child.name == classSymbol.name);
                    if (!isClassCtor && !IsSupportedReturnType(child.type))
                        continue;

                    string baseFunctionName = "";
                    if (!isClassCtor)
                        baseFunctionName = classSymbol.name + "_" + child.name;
                    else
                        baseFunctionName = classSymbol.name + "_Ctor";
                    if (child.isStatic)
                        baseFunctionName += "_Static";

                    // First overload?
                    if (!overloads.ContainsKey(baseFunctionName))
                        overloads[baseFunctionName] = new List<Overload>();

                    // Differentiate function name by parameters
                    string functionName = baseFunctionName;
                    for (int i = 0; i < child.parameters.Count; ++i)
                        functionName += "_" + child.parameters[i].BasicType();
     
                    // Skip if same overload (typically a const variation) already included
                    bool hasSame = false;
                    foreach (Overload o in overloads[baseFunctionName])
                    {
                        if (o.functionName == functionName)
                        {
                            hasSame = true;
                            break;
                        }
                    }
                    if (hasSame)
                        continue;

                    Overload newOverload = new Overload();
                    newOverload.functionName = functionName;
                    newOverload.function = child;
                    newOverload.parameters = child.parameters;
                    overloads[baseFunctionName].Add(newOverload);

                    if (isClassCtor)
                    {
                        tw.WriteLine("duk_ret_t " + functionName + DukSignature());
                        tw.WriteLine("{");

                        // \todo Remove unusable arguments, such as pointers
                        string args = "";
                        for (int i = 0; i < child.parameters.Count; ++i)
                        {
                            tw.WriteLine(Indent(1) + GenerateGetFromStack(child.parameters[i].BasicType(), i, child.parameters[i].name, child.parameters[i].IsAReference())); 
                            if (i > 0)
                                args += ", ";
                            if (child.parameters[i].IsAReference())
                                args += "*";
                                  
                            args += child.parameters[i].name;                                    
                        }
                        tw.WriteLine(Indent(1) + classSymbol.name + "* newObj = new " + classSymbol.name + "(" + args + ");");
                        tw.WriteLine(Indent(1) + GeneratePushConstructorResultToStack(classSymbol.name, "newObj"));
                        tw.WriteLine(Indent(1) + "return 0;");
                        tw.WriteLine("}");
                        tw.WriteLine("");   
                    }          
                    else
                    {
                        tw.WriteLine("duk_ret_t " + functionName + DukSignature());
                        tw.WriteLine("{");
                        string callPrefix = "";
                        if (!child.isStatic)
                        {
                            callPrefix = "thisObj->";
                            tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                        }
                        else
                            callPrefix = classSymbol.name + "::";
                        
                        string args = "";
                        for (int i = 0; i < child.parameters.Count; ++i)
                        {
                            tw.WriteLine(Indent(1) + GenerateGetFromStack(child.parameters[i].BasicType(), i, child.parameters[i].name, child.parameters[i].IsAReference()));
                            if (i > 0)
                                args += ", ";
                            if (child.parameters[i].IsAReference())
                                args += "*";

                            args += child.parameters[i].name;
                        }
                        if (child.type == "void")
                        {
                            tw.WriteLine(Indent(1) + callPrefix + child.name + "(" + args + ");");
                            tw.WriteLine(Indent(1) + "return 0;");
                        }
                        else
                        {
                            tw.WriteLine(Indent(1) + child.type + " ret = " + callPrefix + child.name + "(" + args + ");");
                            tw.WriteLine(Indent(1) + GeneratePushToStack(child.type, "ret"));
                            tw.WriteLine(Indent(1) + "return 1;");
                        }
                        tw.WriteLine("}");
                        tw.WriteLine(""); 
                    }                           
                }
            }
        }

        static void GenerateFunctionSelectors(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads)
        {
            foreach (KeyValuePair<string, List<Overload> > kvp in overloads)
            {
                if (kvp.Value.Count >= 2)
                {
                    tw.WriteLine("duk_ret_t " + kvp.Key + "_Selector" + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + "int numArgs = duk_get_top(ctx);");
                    foreach (Overload o in kvp.Value)
                    {
                        string argCheck = "if (numArgs == " + o.parameters.Count;
                        for (int i = 0; i < o.parameters.Count; ++i)
                            argCheck += " && " + GenerateArgCheck(o.parameters[i], i);
                        argCheck += ")";
                        tw.WriteLine(Indent(1) + argCheck);
                        tw.WriteLine(Indent(2) + "return " + o.functionName + "(ctx);");
                    }
                    tw.WriteLine(Indent(1) + "duk_error(ctx, DUK_ERR_ERROR, \"Could not select function overload\");");
                    tw.WriteLine("}");
                    tw.WriteLine("");   
                }
            }
        }

        static void GenerateFunctionList(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads, bool generateStatic)
        {
            if (!generateStatic)
                tw.WriteLine("const duk_function_list_entry " + classSymbol.name + "_Functions[] = {");
            else
                tw.WriteLine("const duk_function_list_entry " + classSymbol.name + "_StaticFunctions[] = {");
            bool first = true;
            foreach (KeyValuePair<string, List<Overload> > kvp in overloads)
            {
                if (kvp.Value[0].functionName.Contains("Ctor"))
                    continue;

                string prefix = first ? "" : ",";

                if (kvp.Value.Count >= 2)    
                    tw.WriteLine(Indent(1) + prefix + "{\"" + kvp.Value[0].function.name + "\", " + kvp.Key + "_Selector, DUK_VARARGS}");
                else
                    tw.WriteLine(Indent(1) + prefix + "{\"" + kvp.Value[0].function.name + "\", " + kvp.Value[0].functionName + ", " + kvp.Value[0].parameters.Count + "}");

                first = false;
            }

            tw.WriteLine(Indent(1) + ",{nullptr, nullptr, 0}");
            tw.WriteLine("};");
            tw.WriteLine("");  
        }

        static HashSet<string> FindDependencies(Symbol classSymbol)
        {
            HashSet<string> dependencies = new HashSet<string>();
            foreach (Symbol child in classSymbol.children)
            { 
                if (!IsScriptable(child))
                    continue;

                if (child.kind == "function" && !child.name.Contains("operator"))
                {
                    AddDependencyIfValid(dependencies, child.type); // Return type
                    foreach (Parameter p in child.parameters)
                        AddDependencyIfValid(dependencies, p.BasicType());
                }
            }

            return dependencies;                    
        }

        static string DukSignature()
        {
            return "(duk_context* ctx)";
        }

        static string ClassIdentifier(string className)
        {
            return className + "_Id";
        }

        static string SanitateTypeName(string type)
        {
            string t = type.Trim();
            if (t.EndsWith("&") || t.EndsWith("*"))
            {
                t = t.Substring(0, t.Length - 1).Trim();
                if (t.StartsWith("const"))
                    t = t.Substring(5).Trim();
            }
            if (t.EndsWith("const"))
                t = t.Substring(0, t.Length - 5).Trim();
            return t;
        }

        static void AddDependencyIfValid(HashSet<string> dependencyNames, string typeName)
        {
            string t = SanitateTypeName(typeName);
            if (!Symbol.IsPODType(t) && classNames.Contains(t))
                dependencyNames.Add(t);
        }

        static bool IsSupportedReturnType(string typeName)
        {
            string t = SanitateTypeName(typeName);
            return t == "void" || Symbol.IsPODType(t) || classNames.Contains(t);
        }

        static bool IsBadType(string type)
        {
            return type.Contains("bool *") || type.EndsWith("float *") || type.EndsWith("float3 *") || type.Contains("std::") || type.Contains("char*") || type.Contains("char *") || type.Contains("[");
        }

        static public string Indent(int num)
        {
            string s = "";
            int indentSize = 4;
            for (int i = 0; i < num * indentSize; ++i)
                s += " ";
            return s;
        }

        static bool IsScriptable(Symbol s)
        {
            if (s.argList.Contains("["))
                return false;
            if (IsBadType(s.type))
                return false;
            foreach (Parameter p in s.parameters)
                if (IsBadType(p.type) || IsBadType(p.BasicType()))
                    return false;

            foreach (string str in s.Comments())
                if (str.Contains("[noscript]"))
                    return false;
            if (s.returnComment != null && s.returnComment.Contains("[noscript]"))
                return false;
            return true;
        }
    }
}