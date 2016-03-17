// For conditions of distribution and use, see copyright notice in LICENSE

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
        public bool hasDefaultParameters;
        public int numSignificantParameters;
        public bool property;
    }

    class OverloadComparer : IComparer<Overload>
    {
        public int Compare(Overload x, Overload y)
        {
            return y.numSignificantParameters.CompareTo(x.numSignificantParameters);
        }
    }


    struct Property
    {
        public string name;
        public bool readOnly;
    }

    class Program
    {
        static HashSet<string> classNames = new HashSet<string>();
        static List<string> exposeTheseClasses = new List<string>();
        static List<string> dependencyOnlyClasses = new List<string>();
        static HashSet<string> dependencies = new HashSet<string>();
        static HashSet<string> finalizerDependencies = new HashSet<string>();
        static string fileBasePath = "";
        static Dictionary<string, string> classHeaderFiles = new Dictionary<string, string>();
        static Dictionary<string, bool> isRefCounted = new Dictionary<string, bool>();
        static string[] headerFiles = null;

        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("First cmdline parameter should be the absolute path to the root where doxygen generated the documentation XML files.");
                Console.WriteLine("Second cmdline parameter should be the output directory.");
                Console.WriteLine("Third cmdline parameter (optional) should be the root path of the exposed code. If omitted, include paths may not be accurate.");
                Console.WriteLine("Further cmdline parameters can optionally list the classes to be exposed. Otherwise all will be exposed. Prepend _ to class name to only mark it as a dependency or base, but avoid writing bindings for it");
                return;
            }

            CodeStructure s = new CodeStructure();
            s.LoadSymbolsFromDirectory(args[0], true);

            string outputDirectory = args[1];
            try
            {
                Directory.CreateDirectory(outputDirectory);
            }
            catch (Exception)
            {
            }

            if (args.Length > 2)
            {
                fileBasePath = args[2];
                headerFiles = Directory.GetFiles(fileBasePath, "*.h", SearchOption.AllDirectories);
            }

            if (args.Length > 3)
            {
                for (int i = 3; i < args.Length; ++i)
                {
                    string className = args[i];
                    if (className.StartsWith("_"))
                    {
                        className = className.Substring(1);
                        dependencyOnlyClasses.Add(className);
                    }
                    classNames.Add(className);
                    exposeTheseClasses.Add(className);
                }
            }

            foreach (Symbol classSymbol in s.symbolsByName.Values)
            {
                if ((classSymbol.kind == "class" || classSymbol.kind == "struct" || classSymbol.kind == "namespace") && (exposeTheseClasses.Count == 0 || exposeTheseClasses.Contains(StripNamespace(classSymbol.name))))
                {
                    if (IsBadNamespace(ExtractNamespace(classSymbol.name)))
                        continue;

                    string typeName = SanitateTypeName(classSymbol.name);

                    if (!classNames.Contains(typeName))
                        classNames.Add(typeName);

                    isRefCounted[typeName] = IsRefCounted(classSymbol);

                    if (headerFiles != null)
                    {
                        foreach (string str in headerFiles)
                        {
                            if (str.EndsWith(typeName + ".h"))
                            {
                                string sanitated = str.Substring(fileBasePath.Length).Replace('\\', '/');
                                if (sanitated.StartsWith("/"))
                                    sanitated = sanitated.Substring(1);
                                int idx = sanitated.IndexOf(typeName);
                                if (idx == 0 || sanitated[idx - 1] == '/')
                                {
                                    classHeaderFiles[typeName] = sanitated;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            foreach (Symbol classSymbol in s.symbolsByName.Values)
            {
                if (classSymbol.kind == "class" || classSymbol.kind == "struct" || classSymbol.kind == "namespace")
                {
                    if (IsBadNamespace(ExtractNamespace(classSymbol.name)))
                        continue;

                    string typeName = SanitateTypeName(classSymbol.name);
                    if (classNames.Contains(typeName) && !dependencyOnlyClasses.Contains(typeName))
                        GenerateClassBindings(classSymbol, outputDirectory);
                }
            }
        }

        static void GenerateClassBindings(Symbol classSymbol, string outputDirectory)
        {
            string className = StripNamespace(classSymbol.name);
            string namespaceName = ExtractNamespace(classSymbol.name);

            Console.WriteLine("Generating bindings for " + className);

            TextWriter tw = new StreamWriter(outputDirectory + "/" + className + "Bindings.cpp");
            tw.WriteLine("// For conditions of distribution and use, see copyright notice in LICENSE");
            tw.WriteLine("// This file has been autogenerated with BindingsGenerator");
            tw.WriteLine("");
            tw.WriteLine("#include \"StableHeaders.h\"");
            tw.WriteLine("#include \"CoreTypes.h\"");
            tw.WriteLine("#include \"JavaScriptInstance.h\"");
            tw.WriteLine("#include \"LoggingFunctions.h\"");
            {
                string include = FindIncludeForClass(classSymbol.name);
                if (include.Length > 0)
                    tw.WriteLine("#include \"" + include + "\"");
            }
            tw.WriteLine("");
            // Disable bool conversion warnings
            tw.WriteLine("#ifdef _MSC_VER");
            tw.WriteLine("#pragma warning(disable: 4800)");
            tw.WriteLine("#endif");
            tw.WriteLine("");

            // Find dependency classes and add their includes
            dependencies.Clear();
            finalizerDependencies.Clear();
            FindDependencies(classSymbol);
            foreach (string s in dependencies)
            {
                string include = FindIncludeForClass(s);
                if (include.Length > 0)
                    tw.WriteLine("#include \"" + include + "\"");
                /// \todo Add more as needed
                if (s == "Entity" && include.Length == 0)
                    tw.WriteLine("#include \"Entity.h\"");
            }
            tw.WriteLine("");

            // Use namespaces
            if (namespaceName.Length > 0)
            {
                tw.WriteLine("");
                tw.WriteLine("using namespace " + namespaceName + ";");
                if (namespaceName != "Tundra")
                    tw.WriteLine("using namespace Tundra;");
            }
            tw.WriteLine("using namespace std;");
            tw.WriteLine("");

            tw.WriteLine("namespace JSBindings");
            tw.WriteLine("{");
            tw.WriteLine("");
            // Dependencies' type IDs and finalizers (duplicate to avoid externs, possibly across DLL boundaries)
            foreach (string s in dependencies)
            {
                if (!IsRefCounted(s))
                    tw.WriteLine("static const char* " + ClassIdentifier(s) + " = \"" + s + "\";");
            }
            tw.WriteLine("");
            foreach (string s in finalizerDependencies)
            {
                if (!IsRefCounted(s))
                    GenerateFinalizer(s, tw);
            }
            tw.WriteLine("");

            // Own type identifier
            // Included also for refcounted classes, since it's needed when registering the prototype
            tw.WriteLine("static const char* " + ClassIdentifier(className) + " = \"" + className + "\";");
            tw.WriteLine("");

            Dictionary<string, List<Overload> > overloads = new Dictionary<string, List<Overload> >();
            Dictionary<string, List<Overload>> staticOverloads = new Dictionary<string, List<Overload>>();
            List<Property> properties = new List<Property>();
            if (!IsRefCounted(className) && classSymbol.kind != "namespace")
                GenerateFinalizer(classSymbol, tw);
            GeneratePropertyAccessors(classSymbol, tw, properties);
            GenerateMemberFunctions(classSymbol, tw, overloads, false);
            GenerateFunctionSelectors(classSymbol, tw, overloads);
            GenerateMemberFunctions(classSymbol, tw, staticOverloads, true);
            GenerateFunctionSelectors(classSymbol, tw, staticOverloads);
            GenerateFunctionList(classSymbol, tw, overloads, false);
            GenerateFunctionList(classSymbol, tw, staticOverloads, true);
            GenerateExposeFunction(classSymbol, tw, overloads, staticOverloads, properties);
            
            tw.WriteLine("}");
            tw.Close();
        }

        static string FindIncludeForClass(string name)
        {
            name = SanitateTypeName(name);
            // Hack: these classes are in the same include file
            if (name == "AssetReferenceList")
                name = "AssetReference";
            if (name == "RayQueryResult")
                return "IRenderer.h";

            if (classHeaderFiles.ContainsKey(name))
                return classHeaderFiles[name];
            else
                return "";
        }

        static string GenerateGetFromStack(Symbol classSymbol, int stackIndex, string varName)
        {
            string typeName = classSymbol.type;
            if (typeName == null || typeName.Length == 0)
                typeName = classSymbol.name;

            return GenerateGetFromStack(typeName, stackIndex, varName);
        }

        static string GenerateGetFromStack(string typeName, int stackIndex, string varName)
        {
            bool isRawPtr = typeName.EndsWith("*");
            bool isReference = typeName.EndsWith("&");
            bool isPtr = false;
            typeName = SanitateTypeName(typeName);

            if (typeName.EndsWith("Ptr"))
            {
                typeName = typeName.Substring(0, typeName.Length - 3);
                typeName = SanitateTemplateType(typeName);
                isPtr = true;
                isReference = false;
            }

            if (Symbol.IsNumberType(typeName))
            {
                if (typeName == "double")
                    return typeName + " " + varName + " = duk_require_number(ctx, " + stackIndex + ");";
                else if (typeName == "AttributeChange::Type")
                    return typeName + " " + varName + " = (" + typeName + ")(int)duk_require_number(ctx, " + stackIndex + ");"; 
                else
                    return typeName + " " + varName + " = (" + typeName + ")duk_require_number(ctx, " + stackIndex + ");"; 
            }
            else if (typeName == "bool")
                return typeName + " " + varName + " = duk_require_boolean(ctx, " + stackIndex + ");";
            else if (typeName == "string")
                return typeName + " " + varName + " = duk_require_string(ctx, " + stackIndex + ");";
            else if (typeName == "String")
                return typeName + " " + varName + " = duk_require_string(ctx, " + stackIndex + ");";
            else if (typeName == "Variant")
                return typeName + " " + varName + " = GetVariant(ctx, " + stackIndex + ");";
            else if (!Symbol.IsPODType(typeName))
            {
                if (typeName.EndsWith("Vector"))
                {
                    string templateType = typeName.Substring(0, typeName.Length - 6);
                    templateType = SanitateTemplateType(templateType);

                    if (templateType == "String" || templateType == "string")
                        return typeName + " " + varName + " = GetStringVector(ctx, " + stackIndex + ");";
                    else
                    {
                        if (!IsRefCounted(templateType))
                            return typeName + " " + varName + " = GetValueObjectVector<" + templateType + ">(ctx, " + stackIndex + ", " + ClassIdentifier(templateType) + ");";
                        else
                            return typeName + " " + varName + " = GetWeakObjectVector<" + templateType + ">(ctx, " + stackIndex + ");";
                    }
                }

                if (!IsRefCounted(typeName))
                {
                    if (!isRawPtr)
                        return typeName + "& " + varName + " = *GetCheckedValueObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + ");";
                    else
                        return typeName + "* " + varName + " = *GetValueObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + ");";
                }
                else
                {
                    if (!isReference)
                    {
                        if (!isPtr)
                            return typeName + "* " + varName + " = GetWeakObject<" + typeName + ">(ctx, " + stackIndex + ");";
                        else
                            return "SharedPtr<" + typeName + "> " + varName + "(GetWeakObject<" + typeName + ">(ctx, " + stackIndex + "));";
                    }
                    else
                        return typeName + "& " + varName + " = *GetCheckedWeakObject<" + typeName + ">(ctx, " + stackIndex + ");";
                }
            }
            else
                throw new System.Exception("Unsupported type " + typeName + " for GenerateGetVariable()!");
        }

        static string GenerateGetFromStackDefaultValue(string typeName, int stackIndex, string varName, int paramIndex, string defaultValue)
        {
            string ret = GenerateGetFromStack(typeName, stackIndex, varName);
            // Temp-constructed default parameter + reference is not safe, therefore convert to value
            ret = ret.Replace("& ", " ");
            int equalsIdx = ret.IndexOf('=');
            ret = ret.Insert(equalsIdx + 2, "numArgs > " + paramIndex + " ? ");
            ret = ret.Substring(0, ret.Length - 1);
            ret += " : " + defaultValue + ";";
            return ret;
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
            typeName = SanitateTypeName(typeName);
            if (typeName.EndsWith("Ptr"))
            {
                typeName = typeName.Substring(0, typeName.Length - 3);
                typeName = SanitateTemplateType(typeName);
            }

            if (Symbol.IsNumberType(typeName))
                return "duk_push_number(ctx, " + source + ");";
            else if (typeName == "bool")
                return "duk_push_boolean(ctx, " + source + ");";
            else if (typeName == "string")
                return "duk_push_string(ctx, " + source + ".c_str());";
            else if (typeName == "String")
                return "duk_push_string(ctx, " + source + ".CString());";
            else if (typeName == "Variant")
                return "PushVariant(ctx, " + source + ");";
            else if (typeName.EndsWith("Vector"))
            {
                string templateType = typeName.Substring(0, typeName.Length - 6);
                templateType = SanitateTemplateType(templateType);

                if (templateType == "String" || templateType == "string")
                    return "PushStringVector(ctx, " + source + ");";
                else
                {
                    /// \todo This needs the id & finalizer, mark dependencies
                    if (!IsRefCounted(templateType))
                        return "PushValueObjectVector(ctx, " + source + ", " +  ClassIdentifier(templateType) + ", " + templateType + "_Finalizer);";
                    else
                        return "PushWeakObjectVector(ctx, " + source + ");";
                }
            }
            else if (typeName.EndsWith("Map"))
            {
                string templateType = typeName.Substring(0, typeName.Length - 3);
                templateType = SanitateTemplateType(templateType);

                return "PushWeakObjectMap(ctx, " + source + ");";
            }
            else
            {
                if (!IsRefCounted(typeName))
                    return "PushValueObjectCopy<" + typeName + ">(ctx, " + source + ", " + ClassIdentifier(typeName) + ", " + typeName + "_Finalizer);";
                else
                    return "PushWeakObject(ctx, " + source + ");";
            }
        }

        static string GeneratePushConstructorResultToStack(string typeName, string source)
        {
            return "PushConstructorResult<" + typeName + ">(ctx, " + source + ", " + ClassIdentifier(typeName) + ", " + typeName + "_Finalizer);";
        }

        static string GenerateGetThis(Symbol classSymbol, string varName = "thisObj")
        {
            string typeName = classSymbol.type;
            if (typeName == null || typeName.Length == 0)
                typeName = classSymbol.name;
            typeName = StripNamespace(typeName);

            if (!IsRefCounted(classSymbol))
                return typeName + "* " + varName + " = GetThisValueObject<" + typeName + ">(ctx, " + ClassIdentifier(typeName) + ");";
            else
                return typeName + "* " + varName + " = GetThisWeakObject<" + typeName + ">(ctx);";
        }

        static string GenerateArgCheck(Parameter p, int stackIndex)
        {
            string typeName = SanitateTypeName(p.BasicType());
            if (typeName.EndsWith("Ptr"))
            {
                typeName = typeName.Substring(0, typeName.Length - 3);
                typeName = SanitateTemplateType(typeName);
            }

            if (Symbol.IsNumberType(typeName))
                return "duk_is_number(ctx, " + stackIndex + ")";
            else if (typeName == "bool")
                return "duk_is_boolean(ctx, " + stackIndex + ")";
            else if (typeName == "string" || typeName == "String")
                return "duk_is_string(ctx, " + stackIndex + ")";
            if (!Symbol.IsPODType(typeName))
            {
                // Refcounted object parameters could also be legally null
                if (!IsRefCounted(typeName))
                {
                    if (typeName.EndsWith("Vector"))
                        return "duk_is_object(ctx, " + stackIndex + ")";
                    else
                        return "GetValueObject<" + typeName + ">(ctx, " + stackIndex + ", " + ClassIdentifier(typeName) + ")";
                }
                else
                    return "";
            }
            else
                throw new System.Exception("Unsupported type " + typeName + " for GenerateArgCheck()!");
        }

        static void GenerateFinalizer(Symbol classSymbol, TextWriter tw)
        {
            String typeName = SanitateTypeName(classSymbol.name);
            GenerateFinalizer(typeName, tw);
        }

        static void GenerateFinalizer(string typeName, TextWriter tw)
        {
            tw.WriteLine("static duk_ret_t " + typeName + "_Finalizer" + DukSignature());
            tw.WriteLine("{");
            tw.WriteLine(Indent(1) + "FinalizeValueObject<" + typeName + ">(ctx, " + ClassIdentifier(typeName) + ");");
            tw.WriteLine(Indent(1) + "return 0;");
            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GeneratePropertyAccessors(Symbol classSymbol, TextWriter tw, List<Property> properties)
        {
            string className = StripNamespace(classSymbol.name);

            // \hack MGL Frustum class contains private members inside unnamed unions, which CodeStructure doesn't recognize as private. Skip properties altogether for Frustum.
            if (classSymbol.name == "Frustum")
                return;
            
            foreach (Symbol child in classSymbol.children)
            {
                // Signal wrappers
                if (child.kind == "variable" && child.type.StartsWith("Signal"))
                {
                    Property newProperty;
                    newProperty.name = child.name;
                    newProperty.readOnly = true;

                    string signalType = SanitateSignalType(child.type);
                    List<string> parameters = ExtractSignalParameterTypes(signalType);
                    if (parameters.Count == 1 && parameters[0] == "void")
                        parameters.Clear();

                    bool badParameters = false;
                    foreach (string p in parameters)
                    {
                        if (!IsSupportedType(p))
                        {
                            badParameters = true;
                            break;
                        }
                    }
                    if (badParameters)
                        continue;

                    string wrapperClassName = "SignalWrapper" + "_" + className + "_" + child.name;
                    tw.WriteLine("const char* " + ClassIdentifier(wrapperClassName) + " = \"" + wrapperClassName + "\";");
                    tw.WriteLine("");

                    // Wrapper class definition
                    tw.WriteLine("class " + wrapperClassName);
                    tw.WriteLine("{");
                    tw.WriteLine("public:");
                    tw.WriteLine(Indent(1) + wrapperClassName + "(Object* owner, " + signalType + "* signal) :");
                    tw.WriteLine(Indent(2) + "owner_(owner),");
                    tw.WriteLine(Indent(2) + "signal_(signal)");
                    tw.WriteLine(Indent(1) + "{");
                    tw.WriteLine(Indent(1) + "}");
                    tw.WriteLine("");
                    tw.WriteLine(Indent(1) + "WeakPtr<Object> owner_;");
                    tw.WriteLine(Indent(1) + signalType + "* signal_;");
                    tw.WriteLine("};");
                    tw.WriteLine("");

                    // Receiver class definition
                    string receiverClassName = "SignalReceiver" + "_" + className + "_" + child.name;

                    tw.WriteLine("class " + receiverClassName + " : public SignalReceiver");
                    tw.WriteLine("{");
                    tw.WriteLine("public:");
                    string signatureLine = "void OnSignal(";
                    for (int i = 0; i < parameters.Count; ++i)
                    {
                        if (i > 0)
                            signatureLine += ", ";
                        signatureLine += parameters[i] + " param" + i;
                    }
                    signatureLine += ")";
                    tw.WriteLine(Indent(1) + signatureLine);
                    tw.WriteLine(Indent(1) + "{");
                    tw.WriteLine(Indent(2) + "duk_context* ctx = ctx_;");
                    tw.WriteLine(Indent(2) + "duk_push_global_object(ctx);");
                    tw.WriteLine(Indent(2) + "duk_get_prop_string(ctx, -1, \"_OnSignal\");");
                    tw.WriteLine(Indent(2) + "duk_remove(ctx, -2);"); // Global object
                    tw.WriteLine(Indent(2) + "duk_push_number(ctx, (size_t)key_);"); // Signal identifier for looking up receiver(s)
                    tw.WriteLine(Indent(2) + "duk_push_array(ctx);"); // Parameter array
                    for (int i = 0; i < parameters.Count; ++i)
                    {
                        tw.WriteLine(Indent(2) + GeneratePushToStack(parameters[i], "param" + i));
                        tw.WriteLine(Indent(2) + "duk_put_prop_index(ctx, -2, " + i + ");");
                    }
                    tw.WriteLine(Indent(2) + "bool success = duk_pcall(ctx, 2) == 0;");
                    tw.WriteLine(Indent(2) + "if (!success) LogError(\"[JavaScript] OnSignal: \" + String(duk_safe_to_string(ctx, -1)));");
                    tw.WriteLine(Indent(2) + "duk_pop(ctx);"); // Result
                    tw.WriteLine(Indent(1) + "}");
                    tw.WriteLine("};");
                    tw.WriteLine("");

                    // Finalizer
                    GenerateFinalizer(wrapperClassName, tw);

                    // Connect wrapper function
                    tw.WriteLine("static duk_ret_t " + wrapperClassName + "_Connect" + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + wrapperClassName + "* wrapper = GetThisValueObject<" + wrapperClassName + ">(ctx, " + ClassIdentifier(wrapperClassName) + ");");
                    tw.WriteLine(Indent(1) + "if (!wrapper->owner_) return 0;"); // Check signal owner expiration
                    tw.WriteLine(Indent(1) + "HashMap<void*, SharedPtr<SignalReceiver> >& signalReceivers = JavaScriptInstance::InstanceFromContext(ctx)->SignalReceivers();");
                    tw.WriteLine(Indent(1) + "if (signalReceivers.Find(wrapper->signal_) == signalReceivers.End())");
                    tw.WriteLine(Indent(1) + "{");
                    tw.WriteLine(Indent(2) + receiverClassName + "* receiver = new " + receiverClassName + "();");
                    tw.WriteLine(Indent(2) + "receiver->ctx_ = ctx;");
                    tw.WriteLine(Indent(2) + "receiver->key_ = wrapper->signal_;");
                    tw.WriteLine(Indent(2) + "wrapper->signal_->Connect(receiver, &" + receiverClassName + "::OnSignal);");
                    tw.WriteLine(Indent(2) + "signalReceivers[wrapper->signal_] = receiver;");
                    tw.WriteLine(Indent(1) + "}");
                    tw.WriteLine(Indent(1) + "CallConnectSignal(ctx, wrapper->signal_);");
                    tw.WriteLine(Indent(1) + "return 0;");
                    tw.WriteLine("}");
                    tw.WriteLine("");

                    // Disconnect wrapper function
                    tw.WriteLine("static duk_ret_t " + wrapperClassName + "_Disconnect" + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + wrapperClassName + "* wrapper = GetThisValueObject<" + wrapperClassName + ">(ctx, " + ClassIdentifier(wrapperClassName) + ");");
                    tw.WriteLine(Indent(1) + "if (!wrapper->owner_) return 0;"); // Check signal owner expiration
                    tw.WriteLine(Indent(1) + "CallDisconnectSignal(ctx, wrapper->signal_);");
                    tw.WriteLine(Indent(1) + "return 0;");
                    tw.WriteLine("}");
                    tw.WriteLine("");

                    // Emit wrapper function
                    tw.WriteLine("static duk_ret_t " + wrapperClassName + "_Emit" + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + wrapperClassName + "* wrapper = GetThisValueObject<" + wrapperClassName + ">(ctx, " + ClassIdentifier(wrapperClassName) + ");");
                    tw.WriteLine(Indent(1) + "if (!wrapper->owner_) return 0;"); // Check signal owner expiration
                    for (int i = 0; i < parameters.Count; ++i)
                    {
                        tw.WriteLine(Indent(1) + GenerateGetFromStack(parameters[i], i, "param" + i));
                    }
                    string callLine = "wrapper->signal_->Emit(";
                    for (int i = 0; i < parameters.Count; ++i)
                    {
                        if (i > 0)
                            callLine += ", ";
                        callLine += "param" + i;
                    }
                    callLine += ");";
                    tw.WriteLine(Indent(1) + callLine);
                    tw.WriteLine(Indent(1) + "return 0;");
                    tw.WriteLine("}");
                    tw.WriteLine("");

                    tw.WriteLine("static duk_ret_t " + className + "_Get_" + child.name + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                    tw.WriteLine(Indent(1) + wrapperClassName + "* wrapper = new " + wrapperClassName + "(thisObj, &thisObj->" + child.name + ");");
                    tw.WriteLine(Indent(1) + "PushValueObject(ctx, wrapper, " + ClassIdentifier(wrapperClassName) + ", " + wrapperClassName + "_Finalizer, false);");
                    tw.WriteLine(Indent(1) + "duk_push_c_function(ctx, " + wrapperClassName + "_Connect" + ", DUK_VARARGS);");
                    tw.WriteLine(Indent(1) + "duk_put_prop_string(ctx, -2, \"Connect\");");
                    tw.WriteLine(Indent(1) + "duk_push_c_function(ctx, " + wrapperClassName + "_Disconnect" + ", DUK_VARARGS);");
                    tw.WriteLine(Indent(1) + "duk_put_prop_string(ctx, -2, \"Disconnect\");");
                    tw.WriteLine(Indent(1) + "duk_push_c_function(ctx, " + wrapperClassName + "_Emit" + ", " + parameters.Count + ");");
                    tw.WriteLine(Indent(1) + "duk_put_prop_string(ctx, -2, \"Emit\");");
                    tw.WriteLine(Indent(1) + "return 1;");
                    tw.WriteLine("}");
                    tw.WriteLine("");

                    properties.Add(newProperty);
                }
                // Other POD or non-POD public variables
                else if (child.kind == "variable" && !child.isStatic && IsScriptable(child) && child.visibilityLevel == VisibilityLevel.Public && IsSupportedType(child.type))
                {
                    Property newProperty;
                    newProperty.name = SanitatePropertyName(child.name);
                    newProperty.readOnly = true;

                    // Set accessor
                    if (!child.IsConst())
                    {
                        tw.WriteLine("static duk_ret_t " + className + "_Set_" + newProperty.name + DukSignature());
                        tw.WriteLine("{");
                        tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                        tw.WriteLine(Indent(1) + GenerateGetFromStack(child, 0, child.name));
                        tw.WriteLine(Indent(1) + "thisObj->" + child.name + " = " + child.name + ";");
                        tw.WriteLine(Indent(1) + "return 0;");
                        tw.WriteLine("}");
                        tw.WriteLine("");
                        newProperty.readOnly = false;
                    }

                    // Get accessor
                    {
                        string typeName = SanitateTypeName(child.type);
                        if (typeName.EndsWith("Ptr"))
                        {
                            typeName = typeName.Substring(0, typeName.Length - 3);
                            typeName = SanitateTemplateType(typeName);
                        }

                        if (Symbol.IsPODType(typeName) || IsRefCounted(typeName) || typeName == "String" || typeName == "string" || typeName.Contains("Vector"))
                        {
                            if (typeName.Contains("Vector"))
                            {
                                typeName = typeName.Substring(0, typeName.Length - 6);
                                typeName = SanitateTemplateType(typeName);
                                if (!IsRefCounted(child.type))
                                {
                                    // Add finalizer for the value-type property if not already included
                                    if (!finalizerDependencies.Contains(typeName))
                                    {
                                        GenerateFinalizer(typeName, tw);
                                        finalizerDependencies.Add(typeName);
                                    }
                                }
                            }

                            tw.WriteLine("static duk_ret_t " + className + "_Get_" + newProperty.name + DukSignature());
                            tw.WriteLine("{");
                            tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                            tw.WriteLine(Indent(1) + GeneratePushToStack(child, "thisObj->" + child.name));
                        }
                        else
                        {
                            // Non-POD value variable within a larger object: do not create value copy, but point to the data within the object itself,
                            // so that partial access and modification such as transform.pos.x is possible
                            // Note: this is unsafe, should find a better way
                            tw.WriteLine("static duk_ret_t " + className + "_Get_" + newProperty.name + DukSignature());
                            tw.WriteLine("{");
                            tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                            tw.WriteLine(Indent(1) + "PushValueObject<" + typeName + ">(ctx, &thisObj->" + child.name + ", " + ClassIdentifier(typeName) + ", nullptr, true);");
                        }
                        tw.WriteLine(Indent(1) + "return 1;");
                        tw.WriteLine("}");
                        tw.WriteLine("");
                    }

                    properties.Add(newProperty);
                }
            }
        }

        static void GenerateMemberFunctions(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads, bool generateStatic)
        {
            string className = StripNamespace(classSymbol.name);

            foreach (Symbol child in classSymbol.children)
            {
                if (child.isStatic == generateStatic && child.kind == "function" && !child.name.Contains("operator") && child.visibilityLevel == VisibilityLevel.Public)
                {
                    if (!IsScriptable(child))
                    {
                        Console.WriteLine(child.name + " in class " + classSymbol.name + " is not scriptable");
                        continue;
                    }

                    // \hack Unimplemented MathGeolib functions. Remove these checks when fixed
                    if (className == "float4" && child.name == "Orthogonalize")
                        continue;
                    if (className == "Plane" && child.name == "Distance" && child.parameters.Count == 1 && child.parameters[0].BasicType().Contains("float4"))
                        continue;

                    bool isClassCtor = !child.isStatic && (child.name == className);
                    if (!isClassCtor && !IsSupportedType(child.type))
                    {
                        Console.WriteLine(child.name + " in class " + classSymbol.name + " unsupported return value type " + SanitateTypeName(child.type));
                        continue;
                    }
                    // Bindings convention: refcounted objects like Scene or Component can not be constructed from script, but rather must be acquired from the framework
                    if (isClassCtor && IsRefCounted(classSymbol))
                        continue;

                    bool badParameters = false;
                    for (int i = 0; i < child.parameters.Count; ++i)
                    {
                        string t = child.parameters[i].BasicType();

                        if (!IsSupportedType(t))
                        {
                            Console.WriteLine("Unsupported parameter type " + SanitateTypeName(t) + " in function " + child.name + " of " + className);
                            badParameters = true;
                            break;
                        }
                        // If it's a pointer, it must be one of the exposed classes which is refcounted
                        if (t.Contains('*'))
                        {
                            string st = SanitateTypeName(t);
                            if (!IsRefCounted(st) || !classNames.Contains(st))
                            {
                                Console.WriteLine("Unsupported pointer parameter " + st + " in function " + child.name + " of " + className);
                                badParameters = true;
                                break;
                            }
                        }
                    }
                    if (badParameters)
                    {
                        continue;
                    }

                    string baseFunctionName = "";
                    if (!isClassCtor)
                        baseFunctionName = className + "_" + child.name;
                    else
                        baseFunctionName = className + "_Ctor";
                    if (child.isStatic)
                        baseFunctionName += "_Static";

                    // First overload?
                    if (!overloads.ContainsKey(baseFunctionName))
                        overloads[baseFunctionName] = new List<Overload>();

                    // Differentiate function name by parameters
                    string functionName = baseFunctionName;
                    for (int i = 0; i < child.parameters.Count; ++i)
                        functionName += "_" + SanitateTypeName(child.parameters[i].BasicType()).Replace(':', '_');
     
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
                    newOverload.hasDefaultParameters = false;
                    newOverload.numSignificantParameters = child.parameters.Count;

                    foreach (string str in child.Comments())
                        if (str.Contains("[property]"))
                            newOverload.property = true;

                    for (int i = 0; i < child.parameters.Count; ++i)
                    {
                        if (child.parameters[i].defaultValue != null && child.parameters[i].defaultValue.Length > 0)
                        {
                            newOverload.hasDefaultParameters = true;
                            newOverload.numSignificantParameters = i;
                            break;
                        }
                    }

                    overloads[baseFunctionName].Add(newOverload);

                    if (isClassCtor)
                    {
                        tw.WriteLine("static duk_ret_t " + functionName + DukSignature());
                        tw.WriteLine("{");
                        if (newOverload.hasDefaultParameters)
                            tw.WriteLine(Indent(1) + "int numArgs = duk_get_top(ctx);");

                        // \todo Remove unusable arguments, such as pointers
                        string args = "";
                        for (int i = 0; i < child.parameters.Count; ++i)
                        {
                            if (child.parameters[i].defaultValue == null || child.parameters[i].defaultValue.Length == 0)
                                tw.WriteLine(Indent(1) + GenerateGetFromStack(child.parameters[i].BasicTypeRetainReference(), i, child.parameters[i].name));
                            else
                                tw.WriteLine(Indent(1) + GenerateGetFromStackDefaultValue(child.parameters[i].BasicTypeRetainReference(), i, child.parameters[i].name, i, child.parameters[i].defaultValue));
                            
                            if (i > 0)
                                args += ", ";
                            args += child.parameters[i].name;
                        }
                        tw.WriteLine(Indent(1) + className + "* newObj = new " + className + "(" + args + ");");
                        tw.WriteLine(Indent(1) + GeneratePushConstructorResultToStack(className, "newObj"));
                        tw.WriteLine(Indent(1) + "return 0;");
                        tw.WriteLine("}");
                        tw.WriteLine("");
                    }
                    else
                    {
                        tw.WriteLine("static duk_ret_t " + functionName + DukSignature());
                        tw.WriteLine("{");
                        if (newOverload.hasDefaultParameters)
                            tw.WriteLine(Indent(1) + "int numArgs = duk_get_top(ctx);");

                        string callPrefix = "";
                        if (!child.isStatic)
                        {
                            callPrefix = "thisObj->";
                            tw.WriteLine(Indent(1) + GenerateGetThis(classSymbol));
                        }
                        else
                            callPrefix = className + "::";
                        
                        string args = "";
                        for (int i = 0; i < child.parameters.Count; ++i)
                        {
                            if (child.parameters[i].defaultValue == null || child.parameters[i].defaultValue.Length == 0)
                                tw.WriteLine(Indent(1) + GenerateGetFromStack(child.parameters[i].BasicTypeRetainReference(), i, child.parameters[i].name));
                            else
                                tw.WriteLine(Indent(1) + GenerateGetFromStackDefaultValue(child.parameters[i].BasicTypeRetainReference(), i, child.parameters[i].name, i, child.parameters[i].defaultValue));

                            if (i > 0)
                                args += ", ";
                            args += child.parameters[i].name;
                        }
                        if (child.type == "void")
                        {
                            tw.WriteLine(Indent(1) + callPrefix + child.name + "(" + args + ");");
                            tw.WriteLine(Indent(1) + "return 0;");
                        }
                        else
                        {
                            tw.WriteLine(Indent(1) + SanitateTypeForFunction(child.type) + " ret = " + callPrefix + child.name + "(" + args + ");");
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
                    kvp.Value.Sort(new OverloadComparer());

                    tw.WriteLine("static duk_ret_t " + kvp.Key + "_Selector" + DukSignature());
                    tw.WriteLine("{");
                    tw.WriteLine(Indent(1) + "int numArgs = duk_get_top(ctx);");
                    foreach (Overload o in kvp.Value)
                    {
                        int paramCount = o.parameters.Count;
                        for (int i = 0; i < o.parameters.Count; ++i)
                        {
                            if (o.parameters[i].defaultValue != null && o.parameters[i].defaultValue.Length > 0)
                            {
                                paramCount = i;
                                break;
                            }
                        }

                        string argsCheck = "";
                        if (paramCount == o.parameters.Count)
                            argsCheck = "if (numArgs == " + paramCount;
                        else
                            argsCheck = "if (numArgs >= " + paramCount;

                        for (int i = 0; i < paramCount; ++i)
                        {
                            string argCheck = GenerateArgCheck(o.parameters[i], i);
                            if (argCheck.Length > 0)
                                argsCheck += " && " + argCheck;
                        }
                        argsCheck += ")";
                        tw.WriteLine(Indent(1) + argsCheck);
                        tw.WriteLine(Indent(2) + "return " + o.functionName + "(ctx);");
                    }
                    tw.WriteLine(Indent(1) + "duk_error(ctx, DUK_ERR_ERROR, \"Could not select function overload\");");
                    tw.WriteLine("}");
                    tw.WriteLine("");
                }
            }
        }

        static bool HasFunctions(Dictionary<string, List<Overload> > overloads)
        {
            foreach (KeyValuePair<string, List<Overload>> kvp in overloads)
            {
                // Constructor does not appear in the function list; check if there's anything else
                if (kvp.Value[0].functionName.Contains("Ctor"))
                    continue;
                else
                    return true;
            }

            return false;
        }

        static void GenerateFunctionList(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads, bool generateStatic)
        {
            if (!HasFunctions(overloads))
                return;

            string className = StripNamespace(classSymbol.name);

            if (!generateStatic)
                tw.WriteLine("static const duk_function_list_entry " + className + "_Functions[] = {");
            else
                tw.WriteLine("static const duk_function_list_entry " + className + "_StaticFunctions[] = {");
            bool first = true;
            foreach (KeyValuePair<string, List<Overload> > kvp in overloads)
            {
                if (kvp.Value[0].functionName.Contains("Ctor"))
                    continue;

                string prefix = first ? "" : ",";

                if (kvp.Value.Count >= 2)
                    tw.WriteLine(Indent(1) + prefix + "{\"" + kvp.Value[0].function.name + "\", " + kvp.Key + "_Selector, DUK_VARARGS}");
                else
                    tw.WriteLine(Indent(1) + prefix + "{\"" + kvp.Value[0].function.name + "\", " + kvp.Value[0].functionName + ", " + (kvp.Value[0].hasDefaultParameters ? "DUK_VARARGS" : kvp.Value[0].parameters.Count.ToString()) + "}");

                first = false;
            }

            tw.WriteLine(Indent(1) + ",{nullptr, nullptr, 0}");
            tw.WriteLine("};");
            tw.WriteLine("");  
        }

        static void GenerateExposeFunction(Symbol classSymbol, TextWriter tw, Dictionary<string, List<Overload> > overloads, Dictionary<string, List<Overload> > staticOverloads, List<Property> properties)
        {
            bool hasFunctions = HasFunctions(overloads);
            bool hasStaticFunctions = HasFunctions(staticOverloads);
            string className = StripNamespace(classSymbol.name);

            tw.WriteLine("void Expose_" + className + DukSignature());
            tw.WriteLine("{");
            
            bool hasCtor = false;
            string ctorName = className + "_Ctor";
            if (overloads.ContainsKey(ctorName))
            {
                hasCtor = true;
                if (overloads[ctorName].Count >= 2)
                    ctorName += "_Selector";
            }

            if (hasCtor)
                tw.WriteLine(Indent(1) + "duk_push_c_function(ctx, " + ctorName + ", DUK_VARARGS);");
            else
                tw.WriteLine(Indent(1) + "duk_push_object(ctx);");

            if (hasStaticFunctions)
                tw.WriteLine(Indent(1) + "duk_put_function_list(ctx, -1, " + className + "_StaticFunctions);");
            foreach (Symbol child in classSymbol.children)
            {
                if (child.kind == "enum")
                {
                    foreach (Symbol value in child.children)
                    {
                        tw.WriteLine(Indent(1) + "duk_push_number(ctx, " + value.value + ");");
                        tw.WriteLine(Indent(1) + "duk_put_prop_string(ctx, -2, \"" + value.name + "\");");
                    }
                }
            }
            
            if (properties.Count > 0 || hasFunctions)
            {
                tw.WriteLine(Indent(1) + "duk_push_object(ctx);");
                if (hasFunctions)
                    tw.WriteLine(Indent(1) + "duk_put_function_list(ctx, -1, " + className + "_Functions);");
                foreach (Property p in properties)
                {
                    if (!p.readOnly)
                        tw.WriteLine(Indent(1) + "DefineProperty(ctx, \"" + p.name + "\", " + className + "_Get_" + p.name + ", " + className + "_Set_" + p.name + ");");
                    else
                        tw.WriteLine(Indent(1) + "DefineProperty(ctx, \"" + p.name + "\", " + className + "_Get_" + p.name + ", nullptr);");
                }

                // Check functions which have been annotated as property accessors
                foreach (KeyValuePair<string, List<Overload> > lo in overloads)
                {
                    foreach (Overload o in lo.Value)
                    {
                        if (o.property)
                        {
                            string propertyName = lo.Key.Substring(className.Length + 1);
                            if (propertyName.StartsWith("Is"))
                                propertyName = propertyName.Substring(2);
                            if (propertyName.StartsWith("Get"))
                                propertyName = propertyName.Substring(3);
                            string originalPropertyName = propertyName;
                            propertyName = SanitatePropertyName(propertyName);
                            string getterFunctionName = o.functionName;
                            // Search for the corresponding setter function
                            string setterFunctionName = "";
                            string setterName = className + "_" + "Set" + originalPropertyName;
                            if (overloads.ContainsKey(setterName))
                            {
                                foreach (Overload so in overloads[setterName])
                                {
                                    if (so.parameters.Count == 1)
                                    {
                                        setterFunctionName = so.functionName;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                setterName = className + "_" + "SetIs" + originalPropertyName;
                                if (overloads.ContainsKey(setterName))
                                {
                                    foreach (Overload so in overloads[setterName])
                                    {
                                        if (so.parameters.Count == 1)
                                        {
                                            setterFunctionName = so.functionName;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (setterFunctionName.Length > 0)
                                tw.WriteLine(Indent(1) + "DefineProperty(ctx, \"" + propertyName + "\", " + getterFunctionName + ", " + setterFunctionName + ");");
                            else
                                tw.WriteLine(Indent(1) + "DefineProperty(ctx, \"" + propertyName + "\", " + getterFunctionName + ", nullptr);");
                        }
                    }
                }


                tw.WriteLine(Indent(1) + "duk_put_prop_string(ctx, -2, \"prototype\");");
            }
            
            tw.WriteLine(Indent(1) + "duk_put_global_string(ctx, " + ClassIdentifier(className) + ");");
            tw.WriteLine("}");
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
                    AddDependencyIfValid(classSymbol, child.type, true); // Return type
                    foreach (Parameter p in child.parameters)
                        AddDependencyIfValid(classSymbol, p.BasicType(), false);
                }

                if (child.kind == "variable" && !child.isStatic && IsScriptable(child) && child.visibilityLevel == VisibilityLevel.Public && IsSupportedType(child.type))
                    AddDependencyIfValid(classSymbol, child.type, true);
            }

            return dependencies;
        }

        static string DukSignature()
        {
            return "(duk_context* ctx)";
        }

        static string ClassIdentifier(string className)
        {
            return className + "_ID";
        }

        static bool IsRefCounted(Symbol classSymbol)
        {
            if (classSymbol.name.Contains("JavaScriptInstance") || classSymbol.name.Contains("AssetTransfer") || classSymbol.name.Contains("AssetStorage"))
                return true;

            // We don't have access to Urho includes, but can check for the presence of macros. \todo Better checks or explicit listings of known refcounted classes if needed
            return classSymbol.FindChildByName("URHO3D_OBJECT") != null || classSymbol.FindChildByName("COMPONENT_NAME") != null || classSymbol.FindChildByName("ParentEntity") != null || classSymbol.FindChildByName("DiskSource") != null;
        }

        static bool IsRefCounted(string className)
        {
            if (isRefCounted.ContainsKey(className))
                return isRefCounted[className];
            else if (className == "Entity" || className == "IComponent" || className == "IAsset" || className == "IAssetTransfer" || className == "IAssetStorage" || className == "JavaScriptInstance")
                return true;
            else
                return false;
        }
        
        static string ExtractNamespace(string className)
        {
            int separatorIndex = className.LastIndexOf("::");
            if (separatorIndex > 0 && !className.StartsWith("AttributeChange"))
                return className.Substring(0, separatorIndex);
            else
                return "";
        }

        static bool IsBadNamespace(string namespaceName)
        {
            return namespaceName == "Urho3D" || namespaceName == "Ogre" || namespaceName == "Tundra::Ogre";
        }

        static string StripNamespace(string className)
        {
            int separatorIndex = className.LastIndexOf("::");
            if (separatorIndex > 0 && !className.StartsWith("AttributeChange"))
                return className.Substring(separatorIndex + 2);
            else
                return className;
        }

        static string SanitatePropertyName(string name)
        {
            // Convention: start with lowercase letter
            return Char.ToLower(name[0]) + name.Substring(1);
        }

        static string SanitateTypeName(string type)
        {
            string t = type.Trim().Replace("< ", "<").Replace(" >", ">");
            if (t.StartsWith("const"))
                t = t.Substring(5).Trim();
            if (t.EndsWith("&") || t.EndsWith("*"))
                t = t.Substring(0, t.Length - 1).Trim();
            if (t.EndsWith("const"))
                t = t.Substring(0, t.Length - 5).Trim();
            if (t == "vec")
                t = "float3";
            if (t.StartsWith("Vector<"))
                t = t.Substring(7).Replace(">", "") + "Vector";

            t = StripNamespace(t);
            if (t == "Key" || t == "KeySequence")
                t = "int";
            return t;
        }

        static string SanitateTypeNameNoPtrNoConst(string type)
        {
            string t = type.Trim().Replace("< ", "<").Replace(" >", ">");
            if (t == "vec")
                t = "float3";
            if (t.StartsWith("Vector<"))
                t = t.Substring(7).Replace(">", "") + "Vector";

            return StripNamespace(t);
        }

        static string SanitateSignalType(string type)
        {
            for (;;)
            {
                int idx = type.IndexOf("ARG(");
                if (idx < 0)
                    break;
                int idx2 = type.IndexOf(')', idx);
                if (idx2 < 0)
                    break;
                ++idx2;
                type = type.Replace(type.Substring(idx, idx2 - idx), "");
            }

            return type;
        }

        static string SanitateTemplateType(string typeName)
        {
            if (typeName == "Component")
                typeName = "IComponent";
            if (typeName == "Asset")
                typeName = "IAsset";
            if (typeName == "AssetTransfer")
                typeName = "IAssetTransfer";
            if (typeName == "AssetStorage")
                typeName = "IAssetStorage";
            if (typeName == "AssetBundle")
                typeName = "IAssetBundle";
            return typeName;
        }

        static List<string> ExtractSignalParameterTypes(string type)
        {
            List<string> ret = new List<string>();
            int idx = type.IndexOf('<') + 1;
            int idx2 = type.IndexOf('>');
            type = type.Substring(idx, idx2 - idx);
            string[] strings = type.Split(',');
            foreach (string s in strings)
                ret.Add(SanitateTypeNameNoPtrNoConst(s.Trim()));
            return ret;
        }

        static string SanitateTypeForFunction(string type)
        {
            type = StripNamespace(type);
            type = type.Replace("EntityMap", "Scene::EntityMap");
            type = type.Replace("ComponentVector", "Entity::ComponentVector");
            type = type.Replace("ComponentMap", "Entity::ComponentMap");
            if (type == "vec")
                type = "float3";
            return type;
        }

        static void AddDependencyIfValid(Symbol classSymbol, string typeName, bool isReturnValue)
        {
            string t = SanitateTypeName(typeName);
            if (SanitateTypeName(classSymbol.name) == t)
                return; // Do not add self as dependency
            if (t.EndsWith("Vector"))
            {
                string templateType = t.Substring(0, t.Length - 6);
                t = SanitateTemplateType(templateType);
            }

            if (!Symbol.IsPODType(t) && classNames.Contains(t))
            {
                dependencies.Add(t);
                // Value objects that are returned need the finalizer function
                if (!IsRefCounted(t) && isReturnValue)
                    finalizerDependencies.Add(t);
            }
        }

        static bool IsSupportedType(string typeName)
        {
            string t = SanitateTypeName(typeName);
            if (t.EndsWith("Vector"))
            {
                string templateType = t.Substring(0, t.Length - 6);
                if (templateType == "String")
                    return true;
                templateType = SanitateTemplateType(templateType);
                return classNames.Contains(templateType);
            }
            else if (t.EndsWith("Map") || t.EndsWith("Ptr"))
            {
                string templateType = t.Substring(0, t.Length - 3);
                templateType = SanitateTemplateType(templateType);
                return classNames.Contains(templateType);
            }

            return t == "void" || Symbol.IsPODType(t) || classNames.Contains(t) || t == "string" || t == "String" || t == "Variant";
        }

        static bool IsBadType(string type)
        {
            string t = SanitateTypeName(type);
            if (t == "string" || t == "String")
                return false;
            return type.Contains("bool *") || type.EndsWith("float *") || type.EndsWith("float3 *") || type.EndsWith("vec *") || type.Contains("std::") || type.Contains("char*") || type.Contains("char *") || type.Contains("[");
        }

        /*
        static bool NeedDereference(Parameter p)
        {
            if (p.IsAPointer() || p.BasicType().EndsWith("Ptr"))
                return false;
            if (Symbol.IsPODType(p.BasicType()))
                return false;
            if (p.BasicType().Contains("string") || p.BasicType().Contains("String"))
                return false;
            if (p.BasicType().Contains("Variant"))
                return false;

            return true;
        }

        static bool NeedDereference(string type)
        {
            type = type.Trim();
            if (type.EndsWith("*") || type.EndsWith("Ptr"))
                return false;
            if (Symbol.IsPODType(type))
                return false;
            if (type.Contains("string") || type.Contains("String") || type.Contains("Variant"))
                return false;

            return true;
        }
        */

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