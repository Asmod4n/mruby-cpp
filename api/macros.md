# mruby public C API: macros

Source: /home/user/mruby at commit 8c7d5f35b. Headers read: include/mruby.h and include/mruby/*.h, without include/mruby/internal.h and include/mruby/presym/.
The .c files were read only to learn what a macro reaches.

How to read the table:

- One row for each macro name. Where a name has more than one definition (one per boxing, or one per configuration), the "declared" cell lists every place, and the notes say how the definitions differ.
- "word", "nan" and "no" mean the three boxings: include/mruby/boxing_word.h, boxing_nan.h and boxing_no.h. value.h:267-273 picks one. Word boxing is the default.
- "unchecked: undefined behaviour" means that the macro reads the bits of the value as the type it expects and does not look at the type first.
- "evaluates X more than once" means that an argument with a side effect runs its side effect more than once.
- A pointer that a macro returns into an object becomes invalid when the GC frees that object. The GC does not move objects (no compaction code in src/gc.c), so a live object keeps its address.
- "the arena" is mrb->gc.arena. A new object stays in it until mrb_gc_arena_restore() or the end of the C method call (gc.c:644-671).

| name | declared | expands to | lifetime | buffer | raise | gc | type | notes |
|---|---|---|---|---|---|---|---|---|
| MRB_TASK_CREATED | mruby.h:202 | the enum constant MRB_FIBER_CREATED | none | none | no | no | no argument | Alias for a value of mrb_context.status. |
| MRB_TASK_STOPPED | mruby.h:203 | the enum constant MRB_FIBER_TERMINATED | none | none | no | no | no argument | Alias for a value of mrb_context.status. |
| MRB_BOP_INTEGER | mruby.h:347 | `1u << op`, the bit of mrb_state.bop_redefined for Integer#op | none | none | no | no | op: an enum mrb_bop value; unchecked | The VM sets the bit when Integer#op is redefined (mruby.h:519-529). |
| MRB_BOP_FLOAT | mruby.h:348 | `1u << (MRB_BOP_COUNT + op)`, the bit for Float#op | none | none | no | no | op: an enum mrb_bop value; unchecked | Same use as MRB_BOP_INTEGER. |
| MRB_BOP_NUMERIC | mruby.h:349 | MRB_BOP_INTEGER(op) bitwise-or MRB_BOP_FLOAT(op) | none | none | no | no | op: an enum mrb_bop value; unchecked | Evaluates op twice. |
| MRB_BOP_SYMBOL_EQ_SLOT | mruby.h:350 | the number `2 * MRB_BOP_COUNT` | none | none | no | no | no argument | Index into mrb_state.bop_builtin. |
| MRB_BOP_SYMBOL_EQ | mruby.h:351 | `1u << MRB_BOP_SYMBOL_EQ_SLOT`, the bit for Symbol#== | none | none | no | no | no argument | |
| MRB_BOP_SLOT_COUNT | mruby.h:352 | the number of entries in mrb_state.bop_builtin | none | none | no | no | no argument | Size of the array at mruby.h:529. |
| MRB_BOP_NIL_TRUE_FALSE_EQ | mruby.h:359 | `1u << MRB_BOP_SLOT_COUNT` | none | none | no | no | no argument | Mirrors MRB_FL_CLASS_EQ_DEFINED of nil, true and false (mruby.h:353-358). It indexes nothing. |
| mrb_exc_get | mruby.h:942 | mrb_exc_get_id(mrb, mrb_intern_cstr(mrb, name)) | returns borrowed (a class) | none | yes: ArgumentError when the name is 65535 bytes or longer (symbol.c:121); RuntimeError "symbol table overflow" (symbol.c:461); Exception "exception corrupted" when the constant is missing or not a class (class.c:818-820); Exception "non-exception raised" when the class is not a subclass of Exception (class.c:827); NoMemoryError | yes | mrb: mrb_state*; name: NUL-terminated C string, unchecked | Looks only in Object (variable.c:1339-1342). It does not call const_missing. Interns name as a dynamic symbol (symbol.c:491-493). |
| MRB_ARGS_REQ | mruby.h:1092 | `((mrb_aspec)((n)&0x1f) << 18)` | none | none | no | no | n: integer; unchecked | A value above 31 is cut to its low 5 bits. |
| MRB_ARGS_OPT | mruby.h:1100 | `((mrb_aspec)((n)&0x1f) << 13)` | none | none | no | no | n: integer; unchecked | A value above 31 is cut to its low 5 bits. |
| MRB_ARGS_ARG | mruby.h:1110 | MRB_ARGS_REQ(n1) bitwise-or MRB_ARGS_OPT(n2) | none | none | no | no | n1, n2: integers; unchecked | |
| MRB_ARGS_REST | mruby.h:1113 | `(mrb_aspec)(1 << 12)` | none | none | no | no | no argument | |
| MRB_ARGS_POST | mruby.h:1116 | `((mrb_aspec)((n)&0x1f) << 7)` | none | none | no | no | n: integer; unchecked | |
| MRB_ARGS_KEY | mruby.h:1119 | key count in bits 2-6, bit 1 set when n2 is non-zero | none | none | no | no | n1, n2: integers; unchecked | |
| MRB_ARGS_BLOCK | mruby.h:1124 | `(mrb_aspec)1` | none | none | no | no | no argument | |
| MRB_ARGS_NOBLOCK | mruby.h:1129 | `(mrb_aspec)(1 << 23)` | none | none | no | no | no argument | |
| MRB_ARGS_ANY | mruby.h:1134 | MRB_ARGS_REST() | none | none | no | no | no argument | |
| MRB_ARGS_NONE | mruby.h:1139 | `(mrb_aspec)0` | none | none | no | no | no argument | A C function proc with aspec 0 gets MRB_PROC_NOARG (proc.h:231-233). |
| mrb_strlen_lit | mruby.h:1295 | `(sizeof(lit "") - 1)` | none | none | no | no | lit: a string literal; anything else is a compile error | Counts the bytes of the literal, embedded NUL bytes included. |
| mrb_intern_lit | mruby.h:1400 | mrb_intern_static(mrb, (lit ""), mrb_strlen_lit(lit)) | keeps pointer lit (symbol.c:378-383 stores the literal itself in mrb->symtbl when it is aligned and NUL-terminated) | none | yes: ArgumentError when lit is 65535 bytes or longer (symbol.c:121); NoMemoryError | yes | mrb: mrb_state*; lit: a string literal (compile error otherwise) | With presym, MRB_SYM() gives the same symbol as a compile-time constant (presym.h:20-40). An unaligned literal is copied into the symbol pool (symbol.c:379-381). |
| mrb_sym2name | mruby.h:1415 | mrb_sym_name(mrb, sym) | returns borrowed (a C string) | returns pointer into the symbol table or into mrb->symbuf; an inline symbol's name is written to mrb->symbuf, so the next call overwrites it (symbol.c:694-701); the name of a dynamic symbol is freed by symbol GC (symbol.c:707-717) | yes: NoMemoryError when the name holds a NUL byte and a dumped String is built in the arena (symbol.c:1296-1309) | yes | mrb: mrb_state*; sym: mrb_sym; an unknown symbol gives NULL | Obsolete name of mrb_sym_name. |
| mrb_sym2name_len | mruby.h:1416 | mrb_sym_name_len(mrb, sym, len) | returns borrowed (a C string) | returns pointer into the symbol table or mrb->symbuf (same limits as mrb_sym2name); stores the length through len | no | no | len: mrb_int*, may be NULL; an unknown symbol gives NULL and length 0 (symbol.c:657-660) | Obsolete name of mrb_sym_name_len. The bytes are not NUL-terminated when the name holds a NUL. |
| mrb_sym2str | mruby.h:1417 | mrb_sym_str(mrb, sym) | returns new (arena) | none | yes: NoMemoryError | yes | sym: mrb_sym; an unknown symbol gives undef (symbol.c:1283) | The String shares the static name buffer when the symbol is static (symbol.c:1291-1292). |
| MRB_OBJ_ALLOC | mruby.h:1436 | `(MRB_VTYPE_TYPEOF(tt)*)mrb_obj_alloc(mrb, tt, klass)` | returns new (arena) | none | yes: TypeError when klass is not a class, module, singleton class or env (gc.c:866-874), when tt differs from the instance type of klass (gc.c:876-883), or when tt is MRB_TT_FREE or below (gc.c:885-886); NoMemoryError; arena overflow error with MRB_GC_FIXED_ARENA (gc.c:647-650) | yes | tt: a constant enumerator of enum mrb_vtype (the cast uses token pasting, so a variable does not compile); klass: struct RClass* or NULL | The object is zero-filled (gc.c:759). Every field past the header is the caller's to fill before the next allocation if the GC must see it. |
| mrb_str_new_lit | mruby.h:1445 | mrb_str_new_static(mrb, (lit), mrb_strlen_lit(lit)) | returns new (arena); keeps pointer lit when the literal is longer than RSTRING_EMBED_LEN_MAX (string.c:156-162) | none | yes: NoMemoryError | yes | lit: a string literal | A long literal gives a MRB_STR_NOFREE String that points at the literal. A write goes through mrb_str_modify, which copies it first (string.c:302-304). |
| mrb_str_new_frozen | mruby.h:1448 | mrb_obj_freeze(mrb, mrb_str_new(mrb,p,len)) | returns new (arena) | none | yes: ArgumentError when len is negative or too large (str_check_length, string.c:167); NoMemoryError | yes | p: const char* (may be NULL); len: mrb_int | Copies the bytes unless p is in read-only data (string.c:171-173). |
| mrb_str_new_cstr_frozen | mruby.h:1449 | mrb_obj_freeze(mrb, mrb_str_new_cstr(mrb,p)) | returns new (arena) | none | yes: NoMemoryError | yes | p: NUL-terminated C string or NULL | |
| mrb_str_new_static_frozen | mruby.h:1450 | mrb_obj_freeze(mrb, mrb_str_new_static(mrb,p,len)) | returns new (arena); keeps pointer p when len is above RSTRING_EMBED_LEN_MAX | none | yes: NoMemoryError | yes | p: const char*; len: mrb_int; a negative len is undefined behaviour, because string.c:156-162 has no length test | p must stay valid as long as the String lives. |
| mrb_str_new_lit_frozen | mruby.h:1451 | mrb_obj_freeze(mrb, mrb_str_new_lit(mrb,lit)) | returns new (arena); keeps pointer lit when long | none | yes: NoMemoryError | yes | lit: a string literal | |
| mrb_method_cache_clear | mruby.h:1506 (only with MRB_NO_METHOD_CACHE) | `((void)0)` | none | none | no | no | mrb: not evaluated | Without MRB_NO_METHOD_CACHE the name is a function (mruby.h:1504). |
| mrb_const_cache_clear | mruby.h:1511 (only with MRB_NO_CONST_CACHE) | `((void)0)` | none | none | no | no | mrb: not evaluated | Without MRB_NO_CONST_CACHE the name is a function (mruby.h:1509). |
| MRB_OPEN_FAILURE | mruby.h:1535 | `(!(mrb) \|\| (mrb)->exc)` | none | none | no | no | mrb: mrb_state* or NULL | Reads mrb_state.exc. Evaluates mrb twice. |
| MRB_OPEN_SUCCESS | mruby.h:1545 | `(!MRB_OPEN_FAILURE(mrb))` | none | none | no | no | mrb: mrb_state* or NULL | Evaluates mrb twice. |
| mrb_toplevel_run_keep | mruby.h:1594 | mrb_top_run(m, p, mrb_top_self(m), k) | returns new (arena) | none | yes: runs Ruby code | yes | m: mrb_state*; p: const struct RProc*; k: mrb_int | The doc at mruby.h:1569-1573 says the current stack is destroyed when called from a C method. |
| mrb_toplevel_run | mruby.h:1595 | mrb_toplevel_run_keep(m, p, 0) | returns new (arena) | none | yes: runs Ruby code | yes | as mrb_toplevel_run_keep | |
| mrb_context_run | mruby.h:1596 | mrb_vm_run(m, p, s, k) | returns new (arena) | none | yes: runs Ruby code | yes | s: mrb_value self | |
| mrb_as_float | mruby.h:1607 (not with MRB_NO_FLOAT) | mrb_float(mrb_ensure_float_type(mrb, x)) | none (returns a C mrb_float) | none | yes: TypeError for nil and for a non-numeric value (object.c:741-776); a Rational, Complex or Bigint converts when its gem is in | yes (word boxing allocates an RFloat for a float that is not inline, etc.c:356-360) | x: any mrb_value; checked: raises TypeError | |
| mrb_to_float | mruby.h:1609 (not with MRB_NO_FLOAT) | mrb_ensure_float_type(mrb, val) | returns new (arena) when an Integer is converted | none | yes: TypeError as mrb_as_float | yes | val: any mrb_value; checked: raises TypeError | Marked obsolete in the header. |
| MRB_RECURSIVE_P | mruby.h:1620 | mrb_recursive_method_p(mrb, mid, obj1, obj2) | none | none | no | no | mid: mrb_sym; obj1, obj2: mrb_value | Walks the call frames from ci[-1] down (kernel.c:192-204). nil as obj2 means "check obj1 only". |
| MRB_RECURSIVE_UNARY_P | mruby.h:1623 | mrb_recursive_method_p(mrb, mid, obj, mrb_nil_value()) | none | none | no | no | as MRB_RECURSIVE_P | |
| MRB_RECURSIVE_BINARY_P | mruby.h:1626 | mrb_recursive_method_p(mrb, mid, obj1, obj2) | none | none | no | no | as MRB_RECURSIVE_P | A nil obj2 is read as "unary" (kernel.c:197). |
| MRB_RECURSIVE_FUNC_P | mruby.h:1629 | mrb_recursive_func_p(mrb, mid, obj, mrb_nil_value()) | none | none | no | no | as MRB_RECURSIVE_P | Starts at ci[-2] to skip the direct caller (kernel.c:214-228). |
| MRB_RECURSIVE_BINARY_FUNC_P | mruby.h:1632 | mrb_recursive_func_p(mrb, mid, obj1, obj2) | none | none | no | no | as MRB_RECURSIVE_P | |
| mrb_gc_arena_save | mruby.h:1635 | `((mrb)->gc.arena_idx)` | none | none | no | no | mrb: mrb_state*; unchecked | Gives an int to pass to mrb_gc_arena_restore. |
| mrb_gc_arena_restore | mruby.h:1636 | `((mrb)->gc.arena_idx = (idx))` | none | none | no | no | idx: int from mrb_gc_arena_save; unchecked | Every object made after the save loses arena protection. A value larger than the current index makes the arena hold stale entries. Nothing checks it. |
| mrb_gc_mark_value | mruby.h:1643 | if val is not immediate, mrb_gc_mark(mrb, mrb_basic_ptr(val)) | none | none | no | no | val: mrb_value; any value | Statement macro (do-while). Evaluates val twice. It is the GC's own mark step (gc.c:1204-1236). The GC has no mark callback for RData (gc.c:1000-1002 marks only the iv table), so outside mark code a call paints an object gray or black at a time the collector does not expect. |
| mrb_field_write_barrier_value | mruby.h:1647 | if val is not immediate, mrb_field_write_barrier(mrb, obj, mrb_basic_ptr(val)) | none | none | no | no | obj: struct RBasic*; val: mrb_value; unchecked | Statement macro. Evaluates val twice. Call it after storing val into a field of obj that the GC marks (gc.c:1971-1990). |
| mrb_convert_type | mruby.h:1653 | mrb_type_convert(mrb, val, type, mrb_intern_lit(mrb, method)) | returns new (arena) or val | none | yes: runs Ruby code (calls the method); TypeError when the result has the wrong type, except that for MRB_TT_STRING it falls back to mrb_any_to_s (object.c:459-470) | yes | type: enum mrb_vtype; tname: not used; method: a string literal | |
| mrb_check_convert_type | mruby.h:1655 | mrb_type_convert_check(mrb, val, type, mrb_intern_lit(mrb, method)) | returns new (arena), val, or nil | none | yes: runs Ruby code | yes | as mrb_convert_type | Gives nil instead of raising when the result has the wrong type (object.c:492-497). |
| MRB_ERROR_SYM | mruby.h:1701; presym.h:67 | mruby.h: mrb_intern_lit(mrb, #sym). presym.h undefines it and redefines it as MRB_SYM(sym) | none (a symbol) | none | no with presym; with the mruby.h form, as mrb_intern_lit | no with presym | sym: a bare identifier; needs a variable named mrb with the mruby.h form | mruby.h includes presym.h at its end (mruby.h:1836-1840), so a C extension sees the presym.h form. |
| E_EXCEPTION | mruby.h:1702 | `mrb->eException_class` | returns borrowed | none | no | no | needs a variable named mrb | Reads mrb_state.eException_class. |
| E_STANDARD_ERROR | mruby.h:1703 | `mrb->eStandardError_class` | returns borrowed | none | no | no | needs a variable named mrb | Reads mrb_state.eStandardError_class. |
| E_RUNTIME_ERROR | mruby.h:1704 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(RuntimeError)) | returns borrowed | none | yes: Exception "exception corrupted" when the constant Object::RuntimeError is missing or not a class; "non-exception raised" when it does not inherit Exception (class.c:814-830) | yes (raise path only) | needs a variable named mrb | Looks up Object's constant table on every use. A program that reassigns the constant changes what the macro gives. Defined at error.c:944. |
| E_TYPE_ERROR | mruby.h:1705 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(TypeError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:946. |
| E_ZERODIV_ERROR | mruby.h:1706 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(ZeroDivisionError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:947. |
| E_ARGUMENT_ERROR | mruby.h:1707 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(ArgumentError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:939. |
| E_INDEX_ERROR | mruby.h:1708 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(IndexError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:951. |
| E_RANGE_ERROR | mruby.h:1709 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(RangeError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:941. |
| E_NAME_ERROR | mruby.h:1710 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(NameError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined in Ruby, mrblib/10error.rb. |
| E_NOMETHOD_ERROR | mruby.h:1711 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(NoMethodError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined in Ruby, mrblib/10error.rb. |
| E_SCRIPT_ERROR | mruby.h:1712 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(ScriptError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:948. |
| E_SYNTAX_ERROR | mruby.h:1713 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(SyntaxError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:950. |
| E_LOCALJUMP_ERROR | mruby.h:1714 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(LocalJumpError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:940. |
| E_REGEXP_ERROR | mruby.h:1715 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(RegexpError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:943. |
| E_FROZEN_ERROR | mruby.h:1716 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(FrozenError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:945. |
| E_NOTIMP_ERROR | mruby.h:1717 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(NotImplementedError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:949. |
| E_KEY_ERROR | mruby.h:1718 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(KeyError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:952. |
| E_FLOATDOMAIN_ERROR | mruby.h:1720 (not with MRB_NO_FLOAT) | mrb_exc_get_id(mrb, MRB_ERROR_SYM(FloatDomainError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR | yes (raise path only) | needs mrb | Defined at error.c:942. |
| E_FIBER_ERROR | mruby.h:1822 | mrb_exc_get_id(mrb, MRB_ERROR_SYM(FiberError)) | returns borrowed | none | yes: as E_RUNTIME_ERROR; it always raises "exception corrupted" in a build without mruby-fiber, because only that gem defines the class (mrbgems/mruby-fiber/src/fiber.c:568) | yes (raise path only) | needs mrb | With presym, MRB_SYM(FiberError) must also be in the presym table or the code does not compile. |
| mrb_string_type | mruby.h:1747 | mrb_ensure_string_type(mrb, str) | returns borrowed (str itself) | none | yes: TypeError when str is not a String (object.c:781-787) | yes (raise path only) | str: any mrb_value; checked: raises TypeError | Obsolete. It does not convert. |
| mrb_to_str | mruby.h:1748 | mrb_ensure_string_type(mrb, str) | returns borrowed (str itself) | none | yes: TypeError when str is not a String | yes (raise path only) | checked: raises TypeError | Obsolete. It does not call to_str. |
| mrb_str_to_str | mruby.h:1750 | mrb_obj_as_string(mrb, str) | returns new (arena) or str | none | yes: runs Ruby code (calls to_s for an object that is not a String, Symbol, Integer or a built-in kind, string.c:3132-3139) | yes | any mrb_value | Obsolete. |
| mrb_as_int | mruby.h:1755 | mrb_integer(mrb_ensure_int_type(mrb, val)) | none (returns a C mrb_int) | none | yes: TypeError when val is not numeric (object.c:678); RangeError for a NaN or infinite Float (numeric.c:1952-1953); RangeError for a Bigint that does not fit mrb_int (bigint.c:5445-5446); a Rational or Complex converts when its gem is in | yes | val: any mrb_value; checked: raises TypeError | A Float is truncated to an Integer (object.c:658-660). |
| mrb_to_integer | mruby.h:1757 | mrb_ensure_int_type(mrb, val) | returns new (arena) when a conversion happens | none | yes: as mrb_as_int | yes | checked: raises TypeError | Obsolete. |
| mrb_to_int | mruby.h:1758 | mrb_ensure_int_type(mrb, val) | returns new (arena) when a conversion happens | none | yes: as mrb_as_int | yes | checked: raises TypeError | Obsolete. |
| mrb_int | mruby.h:1776 | mrb_as_int(mrb, val) | none | none | yes: as mrb_as_int | yes | checked: raises TypeError | Obsolete. The same name is also the C type mrb_int (value.h:76, 85); the macro is function-like, so only `mrb_int(` expands. |
| mrb_alloca | mruby.h:1827 | mrb_temp_alloc(mrb, size) | returns new (arena): the memory hangs off a hidden String in the arena and is freed when the GC frees that String (gc.c:420-425) | none | yes: NoMemoryError; arena overflow error with MRB_GC_FIXED_ARENA (gc.c:647-650) | yes | size: size_t | The block lives only while the arena keeps the hidden String. After mrb_gc_arena_restore past it, the next GC can free it. |
| MRB_FLAGS_MASK | value.h:95 | `(~(~0U << (width)) << (shift))` | none | none | no | no | shift, width: unsigned ints; a width of 32 or more is undefined behaviour (shift of a 32-bit value) | Helper for the other MRB_FLAGS_* macros. |
| MRB_FLAGS_GET | value.h:96 | `(((b) >> (s)) & MRB_FLAGS_MASK(0, w))` | none | none | no | no | b: an integer field; unchecked | Reads a bit field out of a flags word. |
| MRB_FLAGS_SET | value.h:97 | `(b) = MRB_FLAGS_ZERO(b,s,w) \| MRB_FLAGS_MAKE(s,w,n)` | none | modifies b; no frozen check | no | no | b: an lvalue; n is masked to w bits | Evaluates b twice. Used on RBasic.flags, so on an object it writes the header with no frozen check. |
| MRB_FLAGS_ZERO | value.h:98 | `((b) & ~MRB_FLAGS_MASK(s, w))` | none | none | no | no | b: integer | |
| MRB_FLAGS_MAKE | value.h:99 | `(((n) & MRB_FLAGS_MASK(0, w)) << (s))` | none | none | no | no | n: integer | |
| MRB_FLAG_ON | value.h:100 | `((b) \|= MRB_FLAGS_MASK(s, 1))` | none | modifies b; no frozen check | no | no | b: an lvalue | |
| MRB_FLAG_OFF | value.h:101 | `((b) &= ~MRB_FLAGS_MASK(s, 1))` | none | modifies b; no frozen check | no | no | b: an lvalue | |
| MRB_FLAG_CHECK | value.h:102 | `(!!((b) & MRB_FLAGS_MASK(s, 1)))` | none | none | no | no | b: integer | Gives 0 or 1. |
| MRB_NAN_SERIAL_MAX | value.h:134 (nan); value.h:137 (MRB_USE_FLOAT32); value.h:141 (others); none with word boxing or MRB_NO_FLOAT | the largest NaN serial count the payload can hold | none | none | no | no | no argument | Each NaN made gets a count from mrb_state.nan_serial (etc.c:103-107). |
| MRB_NAN_QUIET_BIT | value.h:138 (MRB_USE_FLOAT32); value.h:142 (others); not with nan or word boxing | the quiet bit of the float payload | none | none | no | no | no argument | Used by the static inline mrb_nan_serialize (value.h:153-168). |
| MRB_VTYPE_FOREACH | value.h:189 | an X-macro: calls f(tt, C type, Ruby class name) once for each enum mrb_vtype | none | none | no | no | f: a macro of three parameters | Lists 31 types. MRB_TT_STRUCT maps to struct RArray (value.h:213). |
| MRB_TT_DATA | value.h:231 | MRB_TT_CDATA | none | none | no | no | no argument | Obsolete name. |
| MRB_VTYPE_TYPEOF | value.h:233 | the token `MRB_TYPEOF_##tt`, a typedef made at value.h:235-236 | none | none | no | no | tt: an enumerator token, not a variable | MRB_TYPEOF_MRB_TT_STRING is struct RString, and so on. The types for false, true, symbol, undef and free are void. |
| MRB_TT_FIXNUM | value.h:240 | MRB_TT_INTEGER | none | none | no | no | no argument | Compatibility name. |
| mrb_immediate_p | value.h:284 (generic, used by no boxing); boxing_word.h:230, 232; boxing_nan.h:176 | true when the value carries no heap object. no: `mrb_type(o) <= MRB_TT_CPTR`. word: low tag bits set or the word is nil (or, without inline float, any word up to MRB_Qundef). nan: a float, a non-object tag, or the word 0 | none | none | no | no | o: any mrb_value | The answer changes with the boxing. no: Float, Integer and CPTR values are immediate. word: a heap RFloat, RInteger or RCptr is not immediate. nan: a heap RInteger (MRB_INT64) is not immediate. Evaluates o more than once. |
| mrb_integer_p | value.h:287; boxing_word.h:256 | true for an Integer that fits mrb_int. word: a tagged fixnum or a heap RInteger | none | none | no | no | any mrb_value | A Bigint gives false; use mrb_bigint_p for that. |
| mrb_fixnum_p | value.h:290; boxing_word.h:255; boxing_nan.h:181 | no: the same as mrb_integer_p. word and nan: only an Integer stored in the value word itself | none | none | no | no | any mrb_value | Under word and nan boxing an mrb_int outside MRB_FIXNUM_MIN..MAX is a heap RInteger and gives false here. |
| mrb_symbol_p | value.h:293; boxing_word.h:257 | true for a Symbol | none | none | no | no | any mrb_value | |
| mrb_undef_p | value.h:296; boxing_word.h:258 | true for the internal undef value | none | none | no | no | any mrb_value | |
| mrb_nil_p | value.h:299; boxing_word.h:259; boxing_nan.h:177 | true for nil only. no: type FALSE and value.i 0. word and nan: the word is 0 | none | none | no | no | any mrb_value | |
| mrb_false_p | value.h:302; boxing_word.h:260; boxing_nan.h:180 | true for false only, never for nil | none | none | no | no | any mrb_value | |
| mrb_true_p | value.h:305; boxing_word.h:261 | true for true only | none | none | no | no | any mrb_value | |
| mrb_float_p | value.h:309, 311; boxing_word.h:264, 268, 271; boxing_nan.h:80 | true for a Float. word: an inline float tag or a heap RFloat | none | none | no | no | any mrb_value | FALSE under MRB_NO_FLOAT. |
| mrb_array_p | value.h:315; boxing_word.h:273 | true when the type is MRB_TT_ARRAY | none | none | no | no | any mrb_value | A Struct (MRB_TT_STRUCT) gives false although it is an RArray. |
| mrb_string_p | value.h:318; boxing_word.h:274 | true when the type is MRB_TT_STRING | none | none | no | no | any mrb_value | |
| mrb_hash_p | value.h:321; boxing_word.h:275 | true when the type is MRB_TT_HASH | none | none | no | no | any mrb_value | |
| mrb_cptr_p | value.h:324; boxing_word.h:276 | true when the type is MRB_TT_CPTR | none | none | no | no | any mrb_value | |
| mrb_exception_p | value.h:327; boxing_word.h:277 | true when the type is MRB_TT_EXCEPTION | none | none | no | no | any mrb_value | |
| mrb_free_p | value.h:330; boxing_word.h:278 | true when the type is MRB_TT_FREE | none | none | no | no | any mrb_value | A freed object (gc.c:1392). A value that holds a freed object is already a bug. |
| mrb_object_p | value.h:333; boxing_word.h:279 | true when the type is MRB_TT_OBJECT | none | none | no | no | any mrb_value | |
| mrb_class_p | value.h:336; boxing_word.h:280 | true when the type is MRB_TT_CLASS | none | none | no | no | any mrb_value | A module or a singleton class gives false. |
| mrb_module_p | value.h:339; boxing_word.h:281 | true when the type is MRB_TT_MODULE | none | none | no | no | any mrb_value | |
| mrb_iclass_p | value.h:342; boxing_word.h:282 | true when the type is MRB_TT_ICLASS | none | none | no | no | any mrb_value | |
| mrb_sclass_p | value.h:345; boxing_word.h:283 | true when the type is MRB_TT_SCLASS | none | none | no | no | any mrb_value | |
| mrb_proc_p | value.h:348; boxing_word.h:284 | true when the type is MRB_TT_PROC | none | none | no | no | any mrb_value | |
| mrb_range_p | value.h:351; boxing_word.h:285 | true when the type is MRB_TT_RANGE | none | none | no | no | any mrb_value | |
| mrb_env_p | value.h:354; boxing_word.h:286 | true when the type is MRB_TT_ENV | none | none | no | no | any mrb_value | |
| mrb_data_p | value.h:357; boxing_word.h:287 | true when the type is MRB_TT_CDATA | none | none | no | no | any mrb_value | |
| mrb_fiber_p | value.h:360; boxing_word.h:288 | true when the type is MRB_TT_FIBER | none | none | no | no | any mrb_value | |
| mrb_istruct_p | value.h:363; boxing_word.h:289 | true when the type is MRB_TT_ISTRUCT | none | none | no | no | any mrb_value | |
| mrb_break_p | value.h:366; boxing_word.h:290 | true when the type is MRB_TT_BREAK | none | none | no | no | any mrb_value | |
| mrb_bool | value.h:369; boxing_word.h:253 | Ruby truth: false only for nil and false | none | none | no | no | any mrb_value | The same name is also the C type mrb_bool (value.h:35, 45, 47); only `mrb_bool(` expands. |
| mrb_test | value.h:371 | mrb_bool(o) | none | none | no | no | any mrb_value | |
| mrb_bigint_p | value.h:374, 376 | MRB_USE_BIGINT: the type is MRB_TT_BIGINT. Otherwise FALSE | none | none | no | no | any mrb_value | |
| mrb_ro_data_p | value.h:524 (only without a link-time check) | FALSE | none | none | no | no | p: not evaluated | With MRB_USE_ETEXT_RO_DATA_P or on Apple it is a static inline function (value.h:499-520). It decides whether mrb_str_new keeps the pointer instead of a copy (string.c:171-173). |
| MRB_OBJECT_HEADER | object.h:10 | the five header fields: `struct RClass *c; enum mrb_vtype tt:8; unsigned gc_color:3; unsigned frozen:1; uint32_t flags:20` | none | none | no | no | no argument | Every object struct begins with it. See structs.md. |
| MRB_FLAG_TEST | object.h:17 | `((obj)->flags & (flag))` | none | none | no | no | obj: a pointer to an object struct; unchecked | Takes a pointer, not an mrb_value. |
| mrb_basic_ptr | object.h:22 | `((struct RBasic*)(mrb_ptr(v)))` | returns borrowed | returns pointer into v (the object itself) | no | no | v: mrb_value of a heap object; unchecked: undefined behaviour for an immediate value | No boxing: mrb_ptr reads value.p, so for an immediate value it gives garbage. |
| MRB_OBJ_IS_FROZEN | object.h:24 | the number 1 | none | none | no | no | no argument | Value for the frozen bit in a static object initializer (class.c:5160). |
| mrb_frozen_p | object.h:25 | `((o)->frozen)` | none | none | no | no | o: a pointer to an object struct; unchecked | Takes a pointer, not an mrb_value. mrb_check_frozen_value is the form for a value (error.c:712-717). |
| MRB_FL_OBJ_SHAPED | object.h:31 | `(1 << 5)` | none | none | no | no | no argument | Bit 5 of RObject.flags. On 32-bit the same bit is part of MRB_HASH_AR_EA_N_USED (object.h:28-30). |
| MRB_OBJ_SHAPED_P | object.h:32 | true when tt is MRB_TT_OBJECT and MRB_FL_OBJ_SHAPED is set | none | none | no | no | o: struct pointer; unchecked | When it is true, RObject.iv is not an iv_tbl pointer (object.h:27). |
| mrb_obj_ptr | object.h:38 | `((struct RObject*)(mrb_ptr(v)))` | returns borrowed | returns pointer into v | no | no | v: mrb_value of a heap object; unchecked: undefined behaviour otherwise | The core casts any heap object to RObject this way. Only the header is common to all. |
| mrb_special_const_p | object.h:40 | mrb_immediate_p(x) | none | none | no | no | any mrb_value | |
| MRB_FIXNUM_SHIFT | boxing_word.h:118, 120; boxing_no.h:193 | word: 0 on 64-bit with MRB_INT32, else WORDBOX_FIXNUM_SHIFT (1). no: 0 | none | none | no | no | no argument | Not defined under nan boxing. |
| MRB_SYMBOL_SHIFT | boxing_word.h:122; boxing_no.h:194 | word: WORDBOX_SYMBOL_SHIFT. no: 0 | none | none | no | no | no argument | Not defined under nan boxing. |
| MRB_FIXNUM_MIN | boxing_word.h:125, 128; boxing_nan.h:18; boxing_no.h:195 | the smallest Integer that the value word holds without a heap object | none | none | no | no | no argument | word: INT64_MIN or INT32_MIN shifted right by MRB_FIXNUM_SHIFT. nan: INT32_MIN. no: MRB_INT_MIN. |
| MRB_FIXNUM_MAX | boxing_word.h:126, 129; boxing_nan.h:19; boxing_no.h:196 | the largest such Integer | none | none | no | no | no argument | As MRB_FIXNUM_MIN. |
| WORDBOX_FIXNUM_BIT_POS | boxing_word.h:132 | 1 | none | none | no | no | no argument | Word boxing only. Internal tag layout. |
| WORDBOX_FIXNUM_SHIFT | boxing_word.h:133 | WORDBOX_FIXNUM_BIT_POS | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_FIXNUM_FLAG | boxing_word.h:134 | `(1 << 0)`, the fixnum tag | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_FIXNUM_MASK | boxing_word.h:135 | 1 | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_IMMEDIATE_MASK | boxing_word.h:139, 157 | 0x03 without inline floats, else 0x07 | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_SYMBOL_BIT_POS | boxing_word.h:140 | 2 | none | none | no | no | no argument | Word boxing without inline floats only. |
| WORDBOX_SYMBOL_SHIFT | boxing_word.h:141, 148, 150 | 2 without inline floats; 32 on 64-bit; 5 on 32-bit | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_SYMBOL_FLAG | boxing_word.h:142, 152 | 2 without inline floats, else 0x1c | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_SYMBOL_MASK | boxing_word.h:143, 153 | 3 without inline floats, else 0x1f | none | none | no | no | no argument | Word boxing only. |
| WORDBOX_FLOAT_FLAG | boxing_word.h:145 | 2 | none | none | no | no | no argument | Word boxing with inline floats only. |
| WORDBOX_FLOAT_MASK | boxing_word.h:146 | 3 | none | none | no | no | no argument | Word boxing with inline floats only. |
| WORDBOX_SET_SHIFT_VALUE | boxing_word.h:160 | `(o).w = ((uintptr_t)(v) << WORDBOX_<n>_SHIFT) \| WORDBOX_<n>_FLAG` | none | modifies o | no | no | o: an mrb_value lvalue; n: the token FIXNUM or SYMBOL; v: unchecked | Bits shifted out at the top are lost. |
| WORDBOX_SHIFT_VALUE_P | boxing_word.h:162 | `((o).w & WORDBOX_<n>_MASK) == WORDBOX_<n>_FLAG` | none | none | no | no | o: mrb_value; n: FIXNUM, SYMBOL or FLOAT | |
| WORDBOX_OBJ_TYPE_P | boxing_word.h:164 | `!mrb_immediate_p(o) && bp->tt == MRB_TT_<n>` | none | none | no | no | o: mrb_value; n: a type name token | Reads the tt of the object. A value that points at freed memory is undefined behaviour. |
| mrb_ptr | boxing_word.h:235; boxing_nan.h:153; boxing_no.h:212 | the object pointer. word: the word as void*. nan: the 64-bit word as a pointer. no: value.p | returns borrowed | returns pointer to the object | no | no | o: mrb_value of a heap object; unchecked: undefined behaviour for an immediate value | no: an lvalue when o is one. |
| mrb_cptr | boxing_word.h:236; boxing_nan.h:154; boxing_no.h:213 | word: `vp->p`, the field of a heap RCptr. nan: the low 48 bits of the word. no: value.p | returns borrowed (the C pointer) | none | no | no | o: a CPTR value; unchecked: undefined behaviour for another type (word reads p of whatever object o points at) | word and no: an lvalue, so `mrb_cptr(v) = p` compiles; in word boxing it changes the shared RCptr object. nan: a pointer above 48 bits is cut (boxing_nan.h:173). |
| mrb_float | boxing_word.h:240, 242; boxing_nan.h:125; boxing_no.h:215 | the C mrb_float. word: mrb_word_boxing_value_float() (inline or heap RFloat) or mrb_rfloat_value(). nan: subtracts the offset from the word. no: value.f | none | none | no | no | o: a Float value; unchecked: undefined behaviour (word: a heap object of another type is read as an RFloat) | Not defined under MRB_NO_FLOAT. |
| mrb_fixnum | boxing_word.h:245; boxing_nan.h:135, 149; boxing_no.h:217 | the integer in the value word. word: arithmetic shift right by WORDBOX_FIXNUM_SHIFT. nan: the low 32 bits. no: value.i | none | none | no | no | o: a fixnum value; unchecked: a value of another type gives a meaningless number | word: for a heap RInteger it gives the pointer bits shifted, not the number. Use mrb_integer for any Integer. |
| mrb_integer | boxing_word.h:251; boxing_nan.h:147, 150; boxing_no.h:218 | the mrb_int of an Integer. word: mrb_integer_func(), which reads ip->i for a heap RInteger (boxing_word.h:246-250). nan with MRB_INT64: mrb_nan_boxing_value_int(), which reads p->i for an object tag (boxing_nan.h:137-146). Otherwise mrb_fixnum | none | none | no | no | o: an Integer value; unchecked: undefined behaviour (an object of another type is read as an RInteger) | A Bigint is not covered: it reads the wrong struct. |
| mrb_symbol | boxing_word.h:252; boxing_nan.h:152; boxing_no.h:219 | the mrb_sym. word: shift right by WORDBOX_SYMBOL_SHIFT. nan: low 32 bits. no: value.sym | none | none | no | no | o: a Symbol value; unchecked: meaningless result otherwise | nan: the cast at boxing_nan.h:152 applies to the mask only, so the type of the result is uint64_t until it is converted. |
| mrb_type | boxing_no.h:220 | `(o).tt` | none | none | no | no | any mrb_value | Word and nan boxing define mrb_type as an inline function (boxing_word.h:307, 323; boxing_nan.h:89). |
| mrb_unboxed_type | boxing_no.h:221 | `(o).tt` | none | none | no | no | any mrb_value | Word and nan boxing define it as an inline function that gives MRB_TT_FALSE for every immediate value. |
| mrb_nb_tt | boxing_nan.h:87 | the 2-bit nan-box tag, bits 48-49 of the word | none | none | no | no | o: mrb_value; meaningful only when mrb_float_p(o) is false | Nan boxing only. |
| NANBOX_SET_MISC_VALUE | boxing_nan.h:123 | NANBOX_SET_VALUE(r, MRB_NANBOX_TT_MISC, (t<<32) \| i) | none | modifies r | no | no | r: mrb_value lvalue | Nan boxing only. |
| NANBOX_SET_VALUE | boxing_nan.h:156 | `(o).u = ((uint64_t)tt << 48) \| (uint64_t)v` | none | modifies o | no | no | o: mrb_value lvalue; v: unchecked, bits above 48 run into the tag | Nan boxing only. Statement macro. |
| BOXNO_SET_VALUE | boxing_no.h:223 | sets (o).tt and the union member attr | none | modifies o | no | no | o: mrb_value lvalue | No boxing only. Statement macro. |
| SET_FLOAT_VALUE | boxing_word.h:293; boxing_nan.h:59; boxing_no.h:241 | word: r = mrb_word_boxing_float_value(mrb, v). nan and no: stores the double, and a NaN gets a serial count from mrb_nan_serial_next(mrb) | word: returns new (arena) when the float is not inline (etc.c:356-360; every NaN, subnormals, exponents out of range) | modifies r | yes (word): NoMemoryError, arena overflow error with MRB_GC_FIXED_ARENA; no (nan, no) | yes (word); no (nan, no) | v: mrb_float; r: mrb_value lvalue | nan: evaluates v up to three times. Not defined under MRB_NO_FLOAT. |
| SET_CPTR_VALUE | boxing_word.h:295; boxing_nan.h:173; boxing_no.h:249 | word: r = mrb_word_boxing_cptr_value(mrb, v), a new heap RCptr (etc.c:471-479). nan: tag CPTR and the low 48 bits. no: tt CPTR and value.p | word: returns new (arena) | modifies r | yes (word): NoMemoryError; no (nan, no) | yes (word); no (nan, no) | v: void* | |
| SET_UNDEF_VALUE | boxing_word.h:296; boxing_nan.h:174; boxing_no.h:250 | stores undef | none | modifies r | no | no | r: lvalue | |
| SET_NIL_VALUE | boxing_word.h:297; boxing_nan.h:160; boxing_no.h:228 | stores nil | none | modifies r | no | no | r: lvalue | |
| SET_FALSE_VALUE | boxing_word.h:298; boxing_nan.h:161; boxing_no.h:229 | stores false | none | modifies r | no | no | r: lvalue | |
| SET_TRUE_VALUE | boxing_word.h:299; boxing_nan.h:162; boxing_no.h:230 | stores true | none | modifies r | no | no | r: lvalue | |
| SET_BOOL_VALUE | boxing_word.h:300; boxing_nan.h:163; boxing_no.h:231 | stores true when b is non-zero, else false | none | modifies r | no | no | b: C truth value | no boxing: b is not parenthesized (`b ? ...`), so an argument such as `x = y` parses wrong. word: evaluates r in both branches. |
| SET_INT_VALUE | boxing_word.h:301; boxing_nan.h:166, 168; boxing_no.h:232 | word, and nan with MRB_INT64: r = mrb_boxing_int_value(mrb, n), a heap RInteger when n is outside the fixnum range (etc.c:493-505). Otherwise stores n directly | word and nan with MRB_INT64: returns new (arena) for a large n | modifies r | yes when it allocates: NoMemoryError | yes when it allocates | n: mrb_int | |
| SET_FIXNUM_VALUE | boxing_word.h:302; boxing_nan.h:170; boxing_no.h:233 | stores n in the value word with no range check | none | modifies r | no | no | n: unchecked; word: bits beyond the fixnum range are lost; nan: cut to 32 bits | Use SET_INT_VALUE or mrb_int_value for a number that may be large. |
| SET_SYM_VALUE | boxing_word.h:303; boxing_nan.h:171; boxing_no.h:247 | stores a symbol | none | modifies r | no | no | v: mrb_sym | |
| SET_OBJ_VALUE | boxing_word.h:304; boxing_nan.h:172; boxing_no.h:248 | stores an object pointer. no: also reads ((struct RObject*)v)->tt | none | modifies r | no | no | v: pointer to a live object; unchecked: no boxing dereferences it | |
| RSTRING_EMBED_LEN_MAX | string.h:19 | `sizeof(void*)*3 + sizeof(void*) - 4 - 1` as mrb_int: 27 on 64-bit, 11 on 32-bit | none | none | no | no | no argument | A String of this many bytes or fewer is stored inside the object (string.c:149-151). |
| RSTR_SET_TYPE | string.h:41 | clears the type and embedded-length bits of s->flags and sets MRB_STR_<type> | none | modifies s->flags; no frozen check | no | no | s: struct RString*; type: the token NORMAL, SHARED, FSHARED, NOFREE or EMBED | Changes how every other macro reads the union. The core calls it only right after it fills the union (string.c:82, 99, 108). |
| MRB_STR_NORMAL | string.h:43 | 0 | none | none | no | no | no argument | Heap buffer owned by the String. |
| MRB_STR_SHARED | string.h:44 | 1 | none | none | no | no | no argument | Buffer shared through mrb_shared_string with a reference count. |
| MRB_STR_FSHARED | string.h:45 | 2 | none | none | no | no | no argument | Buffer borrowed from a frozen String in as.heap.aux.fshared. |
| MRB_STR_NOFREE | string.h:46 | 4 | none | none | no | no | no argument | Buffer that the String does not own (a C literal or read-only data). |
| MRB_STR_EMBED | string.h:47 | 8 | none | none | no | no | no argument | Bytes stored in the object. |
| MRB_STR_TYPE_MASK | string.h:48 | 15 | none | none | no | no | no argument | Bits 0-3 of RString.flags. |
| MRB_STR_EMBED_LEN_SHIFT | string.h:68 | 4 | none | none | no | no | no argument | |
| MRB_STR_EMBED_LEN_BITS | string.h:69 | 5 | none | none | no | no | no argument | |
| MRB_STR_EMBED_LEN_MASK | string.h:70 | bits 4-8 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_SHIFT | string.h:74 | 9 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_BITS | string.h:75 | 2 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_MASK | string.h:76 | bits 9-10 | none | none | no | no | no argument | |
| MRB_STR_ENCODING_SHIFT | string.h:81 | 11 | none | none | no | no | no argument | |
| MRB_STR_ENCODING_BITS | string.h:82 | 2 | none | none | no | no | no argument | |
| MRB_STR_ENCODING_MASK | string.h:83 | bits 11-12 | none | none | no | no | no argument | |
| RSTR_EMBED_P | string.h:85 | `((s)->flags & MRB_STR_EMBED)` | none | none | no | no | s: struct RString*; unchecked | |
| RSTR_SET_EMBED_FLAG | string.h:86 | sets MRB_STR_EMBED in s->flags | none | modifies s->flags; no frozen check | no | no | s: struct RString* | Does not clear the other type bits. After it, as.heap is read as embedded bytes. |
| RSTR_SET_EMBED_LEN | string.h:92 | writes n into bits 4-8 of s->flags | none | modifies s->flags; no frozen check | no | no | n: size; only mrb_assert checks n <= RSTRING_EMBED_LEN_MAX, and only with MRB_DEBUG | n is not masked (string.h:87-91): a value above 31 overwrites the coderange and encoding bits. Statement macro. |
| RSTR_SET_LEN | string.h:98 | the embedded length or as.heap.len, by the embed flag | none | modifies s; no frozen check | no | no | s: struct RString*; n: unchecked | Does not write the NUL byte, does not check capacity. mrb_str_resize is the checked form (string.c:1588-1603). Evaluates s more than once. |
| RSTR_EMBED_PTR | string.h:106 | `((struct RStringEmbed*)(s))->ary` | none | returns pointer into s (inside the object); invalid when the GC frees s or when the String leaves embedded form (string.c:202-205) | no | no | s: struct RString*; meaningful only when RSTR_EMBED_P(s) | |
| RSTR_EMBED_LEN | string.h:107 | bits 4-8 of s->flags as mrb_int | none | none | no | no | s: struct RString*; meaningful only when embedded | |
| RSTR_EMBEDDABLE_P | string.h:109 | `((len) <= RSTRING_EMBED_LEN_MAX)` | none | none | no | no | len: integer | A negative len gives true. |
| RSTR_PTR | string.h:111 | RSTR_EMBED_PTR(s) when embedded, else s->as.heap.ptr | none | returns pointer into s; see RSTRING_PTR | no | no | s: struct RString*; unchecked | Evaluates s twice. |
| RSTR_LEN | string.h:112 | the embedded length or as.heap.len | none | none | no | no | s: struct RString*; unchecked | Evaluates s twice. |
| RSTR_CAPA | string.h:113 | RSTRING_EMBED_LEN_MAX when embedded, else s->as.heap.aux.capa | none | none | no | no | s: struct RString*; unchecked | Correct only for an embedded or MRB_STR_NORMAL String. For SHARED and FSHARED the union holds a pointer, so the result is that pointer read as a number; for NOFREE it is 0 (string.c:110). |
| RSTR_SHARED_P | string.h:115 | `((s)->flags & MRB_STR_SHARED)` | none | none | no | no | s: struct RString* | |
| RSTR_FSHARED_P | string.h:116 | `((s)->flags & MRB_STR_FSHARED)` | none | none | no | no | s: struct RString* | |
| RSTR_NOFREE_P | string.h:117 | `((s)->flags & MRB_STR_NOFREE)` | none | none | no | no | s: struct RString* | |
| MRB_STR_CODERANGE_UNKNOWN | string.h:124 | 0 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_7BIT | string.h:125 | 1 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_VALID | string.h:126 | 2 | none | none | no | no | no argument | |
| MRB_STR_CODERANGE_BROKEN | string.h:127 | 3 | none | none | no | no | no argument | |
| RSTR_CODERANGE | string.h:142 (MRB_UTF8_STRING); string.h:148 (otherwise) | bits 9-10 of s->flags; MRB_STR_CODERANGE_7BIT without MRB_UTF8_STRING | none | none | no | no | s: struct RString* | A cached answer. mrb_str_modify resets it to UNKNOWN (string.c:1543). The setter is in internal.h:426 only. |
| MRB_STR_ENCODING_DEFAULT | string.h:162 | 0 | none | none | no | no | no argument | |
| MRB_STR_ENCODING_BINARY | string.h:163 | 1 | none | none | no | no | no argument | |
| MRB_STR_ENCODING_UTF8 | string.h:165 (only with MRB_UTF8_STRING) | MRB_STR_ENCODING_DEFAULT | none | none | no | no | no argument | A build without MRB_UTF8_STRING does not define it, so code that names it does not compile there (string.h:158-161). |
| RSTR_ENCODING | string.h:168 | bits 11-12 of s->flags | none | none | no | no | s: struct RString* | The setter is in internal.h:441 only. |
| RSTR_BINARY_P | string.h:170 | RSTR_ENCODING(s) == MRB_STR_ENCODING_BINARY | none | none | no | no | s: struct RString* | |
| RSTR_SINGLE_BYTE_P | string.h:176 | coderange is 7BIT or the String is binary | none | none | no | no | s: struct RString* | Evaluates s twice. An UNKNOWN coderange gives false even for ASCII bytes. |
| mrb_str_ptr | string.h:186 | `((struct RString*)(mrb_ptr(s)))` | returns borrowed | returns pointer to the String object | no | no | s: mrb_value; unchecked: undefined behaviour when s is not a String | |
| RSTRING | string.h:187 | mrb_str_ptr(s) | returns borrowed | returns pointer to the String object | no | no | as mrb_str_ptr | |
| RSTRING_PTR | string.h:188 | RSTR_PTR(RSTRING(s)) | returns borrowed | returns pointer into s. It becomes invalid when the buffer moves or is replaced: mrb_str_modify on a SHARED, FSHARED or NOFREE String (string.c:283-305), mrb_str_resize (string.c:1588-1603), mrb_str_cat and the functions built on it (string.c:4155-4160, resize_capa at string.c:199-212), any Ruby code that changes the String, or the GC freeing it. The bytes after len are not always NUL: a NOFREE buffer promises only len bytes (string.c:3766-3769) | no | no | s: a String value; unchecked: undefined behaviour otherwise | A write through it skips the frozen check and the unsharing: it changes every String that shares the buffer, or writes into read-only data for NOFREE. Call mrb_str_modify(mrb, RSTRING(s)) first; it raises FrozenError (string.c:1540-1544). |
| RSTRING_EMBED_LEN | string.h:189 | RSTR_EMBED_LEN(RSTRING(s)) | none | none | no | no | s: a String value; unchecked | Meaningful only when the String is embedded. |
| RSTRING_LEN | string.h:190 | RSTR_LEN(RSTRING(s)) | none | none | no | no | s: a String value; unchecked: undefined behaviour otherwise | Length in bytes, not characters. Evaluates s more than once. |
| RSTRING_CAPA | string.h:191 | RSTR_CAPA(RSTRING(s)) | none | none | no | no | s: a String value; unchecked | Wrong for SHARED, FSHARED and NOFREE Strings; see RSTR_CAPA. |
| RSTRING_END | string.h:192 | RSTRING_PTR(s) + RSTRING_LEN(s) | none | returns pointer into s one past the last byte; invalid on the same events as RSTRING_PTR | no | no | s: a String value; unchecked | Evaluates s four times. |
| RSTRING_CSTR | string.h:193 | mrb_string_cstr(mrb, s) | returns borrowed | returns pointer into s. It may unshare s to write a NUL after the bytes (string.c:3770-3776), without a frozen check; invalid on the same events as RSTRING_PTR | yes: TypeError when s is not a String (string.c:3760); ArgumentError "string contains null byte" (string.c:306-312); NoMemoryError when it unshares | yes (when it unshares) | s: any mrb_value; checked: raises TypeError | It writes to the buffer of a frozen String too; that is a change of storage, not of content. |
| mrb_str_index_lit | string.h:201 | `mrb_str_index(mrb, str, lit, mrb_strlen_lit(lit), off);` | none | none | no | no | str: a String value; unchecked; lit: a string literal | The expansion ends with a semicolon, so the macro cannot stand inside an expression such as an if condition. Gives a byte offset or -1 (string.c:1209-1230). |
| mrb_str_buf_new | string.h:424 | mrb_str_new_capa(mrb, capa) | returns new (arena) | none | yes: ArgumentError for a negative or too large capa (string.c:46-54); NoMemoryError | yes | capa: mrb_int | |
| mrb_string_value_ptr | string.h:431 | RSTRING_PTR(str) | returns borrowed | as RSTRING_PTR | no | no | str: a String value; unchecked; mrb is not used | Obsolete. |
| mrb_string_value_len | string.h:433 | RSTRING_LEN(str) | none | none | no | no | str: a String value; unchecked; mrb is not used | Obsolete. |
| mrb_str_strlen | string.h:435 | `strlen(RSTR_PTR(s))` | none | none | no | no | s: struct RString*, not an mrb_value; unchecked | Stops at the first NUL byte. On a NOFREE or SHARED String with no NUL after len it reads past the String. Needs <string.h>. |
| mrb_str_to_inum | string.h:468 | mrb_str_to_integer(mrb, str, base, badcheck) | returns new (arena) for a Bigint or heap Integer | none | yes: TypeError when str is not a String (string.c:3813); ArgumentError for a bad base or, with badcheck, bad digits | yes | str: any mrb_value; checked: raises TypeError | Obsolete. |
| mrb_str_cat_lit | string.h:504 | mrb_str_cat(mrb, str, lit, mrb_strlen_lit(lit)) | none (returns str) | modifies str; checks frozen (FrozenError from mrb_check_frozen or mrb_str_modify, string.c:4116, 4078, 4090) | yes: FrozenError; ArgumentError "string size too big" (string.c:4126); NoMemoryError | yes | str: a String value; unchecked: undefined behaviour otherwise; lit: a string literal | |
| mrb_str_cat2 | string.h:534 | mrb_str_cat_cstr(mrb, str, ptr) | none (returns str) | modifies str; checks frozen | yes: as mrb_str_cat_lit | yes | str: a String value, unchecked; ptr: C string or NULL | Backward-compatible name. |
| mrb_str_buf_cat | string.h:535 | mrb_str_cat(mrb, str, ptr, len) | none (returns str) | modifies str; checks frozen | yes: as mrb_str_cat_lit | yes | str: a String value, unchecked; len: size_t | ptr may point into str itself (string.c:4140-4150). |
| mrb_str_buf_append | string.h:536 | mrb_str_cat_str(mrb, str, str2) | none (returns str) | modifies str; checks frozen | yes: as mrb_str_cat_lit | yes | str, str2: String values; both unchecked (string.c:4213-4216 casts with mrb_str_ptr) | mrb_str_append is the form that checks str2 (string.c:4253-4256). |
| MRB_ARY_EMBED_LEN_MAX | array.h:38 (MRB_ARY_NO_EMBED); array.h:40 | 0 with MRB_ARY_NO_EMBED, else `sizeof(void*)*3/sizeof(mrb_value)` | none | none | no | no | no argument | 3 with 64-bit word boxing, 1 with 64-bit no boxing. MRB_ARY_NO_EMBED is set by itself on some 32-bit builds (array.h:29-35). |
| mrb_ary_ptr | array.h:61 | `((struct RArray*)(mrb_ptr(v)))` | returns borrowed | returns pointer to the Array object | no | no | v: mrb_value; unchecked: undefined behaviour when v is not an Array or Struct | |
| mrb_ary_value | array.h:62 | mrb_obj_value((void*)(p)) | returns borrowed | none | no | no | p: struct RArray*; unchecked | Makes a value from a pointer. Does not protect it from the GC. |
| RARRAY | array.h:63 | `((struct RArray*)(mrb_ptr(v)))` | returns borrowed | returns pointer to the Array object | no | no | as mrb_ary_ptr | |
| ARY_EMBED_P | array.h:66 (MRB_ARY_NO_EMBED); array.h:73 | 0, or `((a)->flags & MRB_ARY_EMBED_MASK)` | none | none | no | no | a: struct RArray*; unchecked | |
| ARY_UNSET_EMBED_FLAG | array.h:67; array.h:74 | `(void)0`, or clears bits 0-2 of a->flags | none | modifies a->flags; no frozen check | no | no | a: struct RArray* | After it the union is read as heap fields, which still hold the embedded values. Only the core may use it. |
| ARY_EMBED_LEN | array.h:68; array.h:75 | 0, or `(a->flags & 7) - 1` | none | none | no | no | a: struct RArray*; meaningful only when embedded | |
| ARY_SET_EMBED_LEN | array.h:69; array.h:76 | `(void)0`, or writes len+1 into bits 0-2 of a->flags | none | modifies a->flags; no frozen check | no | no | len: unchecked; a value above 6 runs into bit 3 | |
| ARY_EMBED_PTR | array.h:70; array.h:77 | `((mrb_value*)NULL)`, or `(a)->as.ary` | none | returns pointer into a (inside the object) | no | no | a: struct RArray* | NULL under MRB_ARY_NO_EMBED. |
| MRB_ARY_EMBED_MASK | array.h:72 (not with MRB_ARY_NO_EMBED) | 7 | none | none | no | no | no argument | Bits 0-2 of RArray.flags hold embedded length + 1. |
| ARY_LEN | array.h:80 | embedded length or (mrb_int)a->as.heap.len | none | none | no | no | a: struct RArray*; unchecked | Evaluates a more than once. |
| ARY_PTR | array.h:81 | ARY_EMBED_PTR(a) or a->as.heap.ptr | none | returns pointer into a; see RARRAY_PTR | no | no | a: struct RArray*; unchecked | Evaluates a twice. |
| RARRAY_LEN | array.h:82 | ARY_LEN(RARRAY(a)) | none | none | no | no | a: an Array value; unchecked: undefined behaviour otherwise | Evaluates a more than once. |
| RARRAY_PTR | array.h:83 | ARY_PTR(RARRAY(a)) | returns borrowed (the elements) | returns pointer into a. It becomes invalid when mrb_ary_modify copies a shared Array (array.c:191-208), when the Array grows (ary_expand_capa, array.c:301), when Ruby code changes it, or when the GC frees it | no | no | a: an Array value; unchecked | A write through it skips the frozen check, the unsharing and the write barrier. Call mrb_ary_modify(mrb, RARRAY(a)) first (array.c:226-230). The GC marks only the first RARRAY_LEN elements (gc.c:1127-1136). |
| RARRAY_GETMEM | array.h:84 | ARY_GETMEM(RARRAY(a), ptr, len) | returns borrowed | stores into ptr and len a pointer into a and its length; same limits as RARRAY_PTR | no | no | a: an Array value; unchecked; ptr: mrb_value* lvalue; len: integer lvalue | Statement macro. Evaluates a once. |
| ARY_SET_LEN | array.h:85 | the embedded length or a->as.heap.len | none | modifies a; no frozen check | no | no | n: unchecked; only mrb_assert checks the embedded limit | Does not check capacity and does not fill new entries. Entries between the old and the new length must hold valid values before the next GC (gc.c:1127-1136). Statement macro. |
| ARY_CAPA | array.h:93 | MRB_ARY_EMBED_LEN_MAX when embedded, else a->as.heap.aux.capa | none | none | no | no | a: struct RArray* | Wrong for a shared Array: aux holds the mrb_shared_array pointer then. |
| MRB_ARY_SHARED | array.h:94 | 256 | none | none | no | no | no argument | Bit 8 of RArray.flags. |
| ARY_SHARED_P | array.h:95 | `((a)->flags & MRB_ARY_SHARED)` | none | none | no | no | a: struct RArray* | |
| ARY_SET_SHARED_FLAG | array.h:96 | sets bit 8 of a->flags | none | modifies a->flags; no frozen check | no | no | a: struct RArray* | After it the GC frees as.heap.aux.shared through mrb_ary_decref (gc.c:1301-1302); a wrong flag frees the wrong thing. |
| ARY_UNSET_SHARED_FLAG | array.h:97 | clears bit 8 of a->flags | none | modifies a->flags; no frozen check | no | no | a: struct RArray* | |
| ARY_GETMEM | array.h:98 | reads length and pointer by the embed flag into len and ptr | none | stores into ptr and len a pointer into a and its length | no | no | a: struct RArray* (evaluated once into a local); ptr, len: lvalues | Statement macro. |
| mrb_ary_ref | array.h:269 | mrb_ary_entry(ary, n) | returns borrowed | none | no | no | ary: an Array value; unchecked: undefined behaviour otherwise; mrb is not used | A negative n counts from the end; an index out of range gives nil (array.c:1784-1794). |
| mrb_hash_ptr | hash.h:35 | `((struct RHash*)(mrb_ptr(v)))` | returns borrowed | returns pointer to the Hash object | no | no | v: a Hash value; unchecked | |
| mrb_hash_value | hash.h:36 | mrb_obj_value((void*)(p)) | returns borrowed | none | no | no | p: struct RHash* | |
| RHASH | hash.h:211 | `((struct RHash*)(mrb_ptr(hash)))` | returns borrowed | returns pointer to the Hash object | no | no | hash: a Hash value; unchecked | |
| MRB_HASH_IB_BIT_BIT | hash.h:213 | 5 | none | none | no | no | no argument | Width of the index-bucket bit count in RHash.flags. |
| MRB_HASH_AR_EA_CAPA_BIT | hash.h:214 | 5 | none | none | no | no | no argument | |
| MRB_HASH_IB_BIT_SHIFT | hash.h:215 | 0 | none | none | no | no | no argument | |
| MRB_HASH_AR_EA_CAPA_SHIFT | hash.h:216 | 0 | none | none | no | no | no argument | Shares bits 0-4 with the IB bit field. On 32-bit the array form keeps its capacity there (hash.c:155-157). |
| MRB_HASH_AR_EA_N_USED_SHIFT | hash.h:217 | 5 | none | none | no | no | no argument | |
| MRB_HASH_SIZE_FLAGS_SHIFT | hash.h:218 | 10 | none | none | no | no | no argument | |
| MRB_HASH_IB_BIT_MASK | hash.h:219 | bits 0-4 | none | none | no | no | no argument | |
| MRB_HASH_AR_EA_CAPA_MASK | hash.h:220 | bits 0-4 | none | none | no | no | no argument | |
| MRB_HASH_AR_EA_N_USED_MASK | hash.h:221 | bits 5-9 | none | none | no | no | no argument | Overlaps MRB_FL_OBJ_SHAPED (bit 5), which is why MRB_OBJ_SHAPED_P also tests tt (object.h:28-32). |
| MRB_HASH_DEFAULT | hash.h:222 | bit 10 | none | none | no | no | no argument | The Hash has a default value or proc, kept in its iv table under ifnone (hash.c:279). |
| MRB_HASH_PROC_DEFAULT | hash.h:223 | bit 11 | none | none | no | no | no argument | The default is a proc. |
| MRB_HASH_HT | hash.h:224 | bit 12 | none | none | no | no | no argument | The Hash uses a hash table (hsh.ht), not an entry array (hsh.ea). |
| MRB_RHASH_DEFAULT_P | hash.h:225 | `(RHASH(hash)->flags & MRB_HASH_DEFAULT)` | none | none | no | no | hash: a Hash value; unchecked | |
| MRB_RHASH_PROCDEFAULT_P | hash.h:226 | `(RHASH(hash)->flags & MRB_HASH_PROC_DEFAULT)` | none | none | no | no | hash: a Hash value; unchecked | |
| mrb_class_ptr | class.h:24 | `((struct RClass*)(mrb_ptr(v)))` | returns borrowed | returns pointer to the class object | no | no | v: a class, module, singleton class or iclass value; unchecked | |
| MRB_FL_CLASS_IS_PREPENDED | class.h:67 | bit 19 | none | none | no | no | no argument | |
| MRB_FL_CLASS_IS_ORIGIN | class.h:68 | bit 18 | none | none | no | no | no argument | |
| MRB_CLASS_ORIGIN | class.h:69 | when c is prepended, walks c->super until a class with MRB_FL_CLASS_IS_ORIGIN and assigns it to c | none | modifies the variable c (not the class) | no | no | c: a struct RClass* lvalue; unchecked | Statement macro. Reads super of each class; a broken chain loops or crashes. |
| MRB_FL_CLASS_IS_INHERITED | class.h:77 | bit 17 | none | none | no | no | no argument | Used by the method cache. |
| MRB_FL_CLASS_EQ_DEFINED | class.h:86 | bit 16 | none | none | no | no | no argument | Set when an ancestor defines its own ==; never cleared (class.h:78-85). |
| MRB_FL_CLASS_IS_REFINEMENT | class.h:89 (MRB_USE_REFINEMENTS) | bit 7 | none | none | no | no | no argument | |
| MRB_CLASS_REFINEMENT_P | class.h:90 (MRB_USE_REFINEMENTS) | `(((c)->flags & MRB_FL_CLASS_IS_REFINEMENT) != 0)` | none | none | no | no | c: struct RClass* | |
| MRB_FL_CLASS_IS_REFINED | class.h:92 (MRB_USE_REFINEMENTS) | bit 8 | none | none | no | no | no argument | |
| MRB_INSTANCE_TT_MASK | class.h:94 | 0x1F | none | none | no | no | no argument | Bits 0-4. The comment at class.h:65 says "0-5"; bit 5 is MRB_FL_OBJ_SHAPED on objects. |
| MRB_SET_INSTANCE_TT | class.h:95 | replaces bits 0-4 of c->flags with (char)tt | none | modifies c->flags; no frozen check | no | no | c: struct RClass*; tt: enum mrb_vtype, not masked: a value above 31 would set bit 5 or more | Sets the type that Class#new allocates (class.c:3667-3685) and that mrb_obj_alloc accepts for the class (gc.c:875-883). A class that wraps RData needs MRB_TT_CDATA here, or Data_Wrap_Struct raises TypeError. Subclasses made later copy it (class.c:4065). Set it before any instance exists. |
| MRB_INSTANCE_TT | class.h:96 | `(enum mrb_vtype)((c)->flags & MRB_INSTANCE_TT_MASK)` | none | none | no | no | c: struct RClass* | 0 means "allocate MRB_TT_OBJECT" for Class#new (class.c:3675-3677). |
| MRB_FL_UNDEF_ALLOCATE | class.h:97 | bit 6 | none | none | no | no | no argument | |
| MRB_UNDEF_ALLOCATOR | class.h:98 | `(mrb_assert(c->tt == MRB_TT_CLASS), c->flags \|= MRB_FL_UNDEF_ALLOCATE)` | none | modifies c->flags; no frozen check | no | no | c: struct RClass*; the class check is mrb_assert only (MRB_DEBUG) | After it Class#new raises TypeError "allocator undefined" (class.c:3678-3680). Evaluates c twice in a debug build. |
| MRB_UNDEF_ALLOCATOR_P | class.h:99 | `((c)->flags & MRB_FL_UNDEF_ALLOCATE)` | none | none | no | no | c: struct RClass* | |
| MRB_DEFINE_ALLOCATOR | class.h:100 | clears MRB_FL_UNDEF_ALLOCATE | none | modifies c->flags; no frozen check | no | no | c: struct RClass* | |
| mrb_mc_clear_by_class | class.h:115 (only with MRB_NO_METHOD_CACHE) | empty | none | none | no | no | arguments not evaluated | With a method cache it is a function (class.h:113) without MRB_API. |
| MRB_MT_READONLY_BIT | class.h:145 | `(1 << 30)` | none | none | no | no | no argument | Bit of mrb_mt_tbl.alloc. |
| MRB_MT_FROZEN_BIT | class.h:146 | `(1 << 29)` | none | none | no | no | no argument | Bit of mrb_mt_tbl.alloc. |
| MRB_MT_FUNC | class.h:147 | `(1 << 24)` | none | none | no | no | no argument | The same bit as MRB_METHOD_FUNC_FL. |
| MRB_MT_PUBLIC | class.h:148 | 0 | none | none | no | no | no argument | |
| MRB_MT_PRIVATE | class.h:149 | `(1 << 25)` | none | none | no | no | no argument | The same bit as MRB_METHOD_PRIVATE_FL. |
| MRB_MT_ENTRY | class.h:152 | an initializer `{ { fn }, sym, flags \| MRB_MT_FUNC }` for mrb_mt_entry | none | none | no | no | fn: mrb_func_t; sym: mrb_sym constant (MRB_SYM); flags: MRB_ARGS_*() optionally with MRB_MT_PRIVATE | For a static const table. A NULL fn makes a "removed" entry (class.h:156-161). |
| MRB_MT_ASPEC | class.h:154 | `((mrb_aspec)((flags) & 0xffffff))` | none | none | no | no | flags: uint32_t | |
| MRB_MT_REMOVED_P | class.h:161 | MRB_MT_FUNC set and val.func NULL | none | none | no | no | e: an mrb_mt_entry (not a pointer) | Evaluates e twice. |
| MRB_MT_INIT_ROM | class.h:173 | mrb_mt_init_rom(mrb, cls, entries, sizeof(entries)/sizeof(entries[0])) | keeps pointer entries (class.c:196-198 stores it in the new table) | modifies cls->mt (pushes a read-only table; freezes the mutable top table, class.c:200-212); no frozen check on cls | yes: NoMemoryError (class.c:186-195) | yes | entries: an array, not a pointer (sizeof gives the count); cls: struct RClass*; unchecked | mrb_mt_init_rom has no MRB_API (class.h:171). entries must live as long as mrb. The function does not clear the method cache (class.c:182-213), so call it before the class is used. |
| Data_Wrap_Struct | data.h:39 | mrb_data_object_alloc(mrb, klass, ptr, type) | returns new (arena); stores ptr and type in the new RData (etc.c:19-27) | none | yes: TypeError when klass's instance type is not MRB_TT_CDATA and klass is not Object (gc.c:876-883); NoMemoryError | yes | klass: struct RClass*; ptr: void*; type: const mrb_data_type* | From now on type->dfree(mrb, ptr) runs when the GC frees the object (gc.c:1340-1346). The GC does not look inside ptr: mrb_values held there are not marked (gc.c:1000-1002). |
| Data_Make_Struct | data.h:42 | Data_Wrap_Struct with NULL, then mrb_malloc(sizeof(strct)), zero-fill, then data_obj->data = sval | returns new (arena) in data_obj; stores the new block in data_obj->data | none | yes: as Data_Wrap_Struct; NoMemoryError from mrb_malloc | yes | strct: a type name; sval: strct* lvalue; data_obj: struct RData* lvalue | Statement macro. If mrb_malloc raises, the object stays with data NULL, so dfree must accept NULL. Uses a static zero object of type strct (data.h:45). |
| RDATA | data.h:49 | `((struct RData*)(mrb_ptr(obj)))` | returns borrowed | returns pointer to the RData object | no | no | obj: a CDATA value; unchecked | |
| DATA_PTR | data.h:50 | `(RDATA(d)->data)` | returns borrowed | returns the C pointer; an lvalue, so a write replaces it with no frozen check | no | no | d: a CDATA value; unchecked: undefined behaviour otherwise | Does not check DATA_TYPE. A write does not free the old pointer; dfree later gets the new one. |
| DATA_TYPE | data.h:51 | `(RDATA(d)->type)` | returns borrowed | an lvalue; a write has no frozen check | no | no | d: a CDATA value; unchecked | |
| DATA_GET_PTR | data.h:54 | `(type*)mrb_data_get_ptr(mrb, obj, dtype)` | returns borrowed | none | yes: TypeError when obj is not CDATA (etc.c:37-39) or its type is not dtype (etc.c:40-51) | yes (raise path only) | obj: any mrb_value; checked: raises TypeError | The pointer may be NULL (an object made by Data_Wrap_Struct with NULL, or before initialize). |
| DATA_CHECK_GET_PTR | data.h:56 | `(type*)mrb_data_check_get_ptr(mrb, obj, dtype)` | returns borrowed | none | no | no | obj: any mrb_value; checked: gives NULL on mismatch (etc.c:60-69) | NULL also when the pointer itself is NULL. |
| mrb_data_check_and_get | data.h:59 | mrb_data_get_ptr(mrb, obj, dtype) | returns borrowed | none | yes: as DATA_GET_PTR | yes (raise path only) | checked: raises TypeError | Obsolete. |
| mrb_get_datatype | data.h:60 | mrb_data_get_ptr(mrb, val, type) | returns borrowed | none | yes: as DATA_GET_PTR | yes (raise path only) | checked: raises TypeError | Obsolete. |
| mrb_check_datatype | data.h:61 | mrb_data_get_ptr(mrb, val, type) | returns borrowed | none | yes: as DATA_GET_PTR | yes (raise path only) | checked: raises TypeError | Obsolete. |
| Data_Get_Struct | data.h:62 | `*(void**)&sval = mrb_data_get_ptr(mrb, obj, type)` | returns borrowed (into sval) | none | yes: as DATA_GET_PTR | yes (raise path only) | sval: a pointer lvalue; checked: raises TypeError | Statement macro. The cast through void** breaks strict aliasing when sval is not a void*. |
| MRB_ENV_SET_LEN | proc.h:39 | writes len into bits 0-7 of e->flags | none | modifies e->flags; no frozen check | no | no | e: struct REnv*; len: cut to 8 bits | The GC marks exactly this many entries of e->stack (gc.c:1029-1032). Core use only. |
| MRB_ENV_LEN | proc.h:40 | bits 0-7 of e->flags as mrb_int | none | none | no | no | e: struct REnv* | |
| MRB_ENV_CLOSE | proc.h:41 | `((e)->cxt = NULL)` | none | modifies e->cxt; no frozen check | no | no | e: struct REnv* | Marks the env as off the VM stack without copying the stack. A direct use leaves e->stack pointing into the VM stack, and the GC then frees it as heap memory (gc.c:1266-1272). mrb_env_unshare copies first (proc.h:85). |
| MRB_ENV_ONSTACK_P | proc.h:42 | `((e)->cxt != NULL)` | none | none | no | no | e: struct REnv* | |
| MRB_ENV_BIDX | proc.h:43 | MRB_FLAGS_GET(e->flags, 8, 6) | none | none | no | no | e: struct REnv* | Block argument index. |
| MRB_ENV_SET_BIDX | proc.h:61 | mrb_env_set_bidx(e, idx), a static inline function | none | modifies e->flags; no frozen check | no | no | idx: cut to 6 bits; mrb_assert(idx < 64) in a debug build | |
| MRB_ENV_GIVEN_CLASS_P | proc.h:68 | MRB_FLAG_CHECK(e->flags, 14) | none | none | no | no | e: struct REnv* | |
| MRB_ENV_SET_GIVEN_CLASS | proc.h:69 | MRB_FLAG_ON(e->flags, 14) | none | modifies e->flags | no | no | e: struct REnv* | |
| MRB_ENV_SET_VISIBILITY | proc.h:70 | MRB_FLAGS_SET(e->flags, 16, 2, vis) | none | modifies e->flags | no | no | vis: masked to 2 bits | Evaluates e twice. |
| MRB_ENV_VISIBILITY | proc.h:71 | MRB_FLAGS_GET(e->flags, 16, 2) | none | none | no | no | e: struct REnv* | |
| MRB_ENV_VISIBILITY_BREAK_P | proc.h:72 | MRB_FLAG_CHECK(e->flags, 18) | none | none | no | no | e: struct REnv* | |
| MRB_ENV_MODFUNC_P | proc.h:73 | MRB_FLAG_CHECK(e->flags, 19) | none | none | no | no | e: struct REnv* | |
| MRB_ENV_SET_MODFUNC | proc.h:74 | MRB_FLAG_ON(e->flags, 19) | none | modifies e->flags | no | no | e: struct REnv* | |
| MRB_ENV_CLEAR_MODFUNC | proc.h:75 | MRB_FLAG_OFF(e->flags, 19) | none | modifies e->flags | no | no | e: struct REnv* | |
| MRB_ENV_COPY_FLAGS_FROM_CI | proc.h:77 | MRB_FLAGS_SET(e->flags, 16, 4, ci->vis) | none | modifies e->flags | no | no | e: struct REnv*; ci: mrb_callinfo* | Reads mrb_callinfo.vis. |
| MRB_ASPEC_REQ | proc.h:102 | bits 18-22 of an aspec | none | none | no | no | a: mrb_aspec | Inverse of MRB_ARGS_REQ. |
| MRB_ASPEC_OPT | proc.h:103 | bits 13-17 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_REST | proc.h:104 | bit 12 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_POST | proc.h:105 | bits 7-11 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_KEY | proc.h:106 | bits 2-6 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_KDICT | proc.h:107 | bit 1 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_BLOCK | proc.h:108 | bit 0 | none | none | no | no | a: mrb_aspec | |
| MRB_ASPEC_NOBLOCK | proc.h:109 | bit 23 | none | none | no | no | a: mrb_aspec | |
| MRB_PROC_CFUNC_FL | proc.h:111 | 128 | none | none | no | no | no argument | Bit 7 of RProc.flags. |
| MRB_PROC_CFUNC_P | proc.h:112 | `(((p)->flags & MRB_PROC_CFUNC_FL) != 0)` | none | none | no | no | p: struct RProc*; unchecked | |
| MRB_PROC_CFUNC | proc.h:113 | `(p)->body.func` | returns borrowed (a function pointer) | none | no | no | p: struct RProc*; meaningful only when MRB_PROC_CFUNC_P(p); otherwise the irep pointer is read as a function pointer | An lvalue. |
| MRB_PROC_STRICT | proc.h:114 | 256 | none | none | no | no | no argument | Lambda semantics. |
| MRB_PROC_STRICT_P | proc.h:115 | `(((p)->flags & MRB_PROC_STRICT) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_ORPHAN | proc.h:116 | 512 | none | none | no | no | no argument | |
| MRB_PROC_ORPHAN_P | proc.h:117 | `(((p)->flags & MRB_PROC_ORPHAN) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_ENVSET | proc.h:118 | 1024 | none | none | no | no | no argument | e.env is valid (not e.target_class). |
| MRB_PROC_ENV_P | proc.h:119 | `(((p)->flags & MRB_PROC_ENVSET) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_ENV | proc.h:120 | p->e.env when MRB_PROC_ENVSET, else NULL | returns borrowed | none | no | no | p: struct RProc* | Evaluates p twice. |
| MRB_PROC_TARGET_CLASS | proc.h:121 | p->e.env->c when MRB_PROC_ENVSET, else p->e.target_class | returns borrowed | none | no | no | p: struct RProc* | Reads the class of the env object (RBasic.c of the env). |
| MRB_PROC_SET_TARGET_CLASS | proc.h:122 | stores tc in p->e.env->c or p->e.target_class, then mrb_field_write_barrier | stores tc in p (or in its env) | modifies p or its env; no frozen check | no | no | p: struct RProc*; tc: struct RClass*; needs a variable named mrb in scope (it is not a parameter) | Statement macro. Evaluates p several times. |
| MRB_PROC_SCOPE | proc.h:132 | 2048 | none | none | no | no | no argument | |
| MRB_PROC_SCOPE_P | proc.h:133 | `(((p)->flags & MRB_PROC_SCOPE) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_LVAR_BOUNDARY_P | proc.h:140 | MRB_PROC_SCOPE_P(p) && !MRB_PROC_ENV_P(p) | none | none | no | no | p: struct RProc* | |
| MRB_PROC_NOARG | proc.h:141 | 4096 | none | none | no | no | no argument | A C function proc whose aspec is MRB_ARGS_NONE(). |
| MRB_PROC_NOARG_P | proc.h:142 | `(((p)->flags & MRB_PROC_NOARG) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_ALIAS | proc.h:143 | 8192 | none | none | no | no | no argument | body.mid holds the original name (class.c:4166-4173). |
| MRB_PROC_CREF | proc.h:152 | 16384 | none | none | no | no | no argument | Same bit as compressed-aspec bit 14 of a C function proc (proc.h:166-175). |
| MRB_PROC_CREF_P | proc.h:153 | `(((p)->flags & MRB_PROC_CREF) != 0)` | none | none | no | no | p: struct RProc* | On a C function proc the bit is an aspec bit, so test MRB_PROC_CFUNC_P first. |
| MRB_PROC_GIVEN | proc.h:162 | 32768 | none | none | no | no | no argument | |
| MRB_PROC_GIVEN_P | proc.h:163 | MRB_PROC_GIVEN set and MRB_PROC_CFUNC_FL clear | none | none | no | no | p: struct RProc* | |
| MRB_PROC_ALIAS_P | proc.h:164 | `(((p)->flags & MRB_PROC_ALIAS) != 0)` | none | none | no | no | p: struct RProc* | |
| MRB_PROC_CASPEC_MASK | proc.h:176 | 0xfc07f, bits 0-6 and 14-19 | none | none | no | no | no argument | Where a C function proc keeps its compressed aspec. |
| MRB_PROC_REFSCOPE_MAX | proc.h:185 (MRB_USE_REFINEMENTS) | 2047 | none | none | no | no | no argument | |
| MRB_PROC_REFSCOPE | proc.h:186 (MRB_USE_REFINEMENTS) | 0 for a C function proc, else the refinement scope index from bits 0-6 and 16-19 | none | none | no | no | p: struct RProc* | |
| MRB_PROC_SET_REFSCOPE | proc.h:188 (MRB_USE_REFINEMENTS) | writes i into bits 0-6 and 16-19 of p->flags | none | modifies p->flags; no frozen check | no | no | i: cut to 11 bits | On a C function proc it overwrites the compressed aspec. |
| mrb_proc_ptr | proc.h:239 | `((struct RProc*)(mrb_ptr(v)))` | returns borrowed | returns pointer to the Proc object | no | no | v: a Proc value; unchecked | |
| mrb_cfunc_env_get | proc.h:249 | mrb_proc_cfunc_env_get(mrb, idx) | returns borrowed | none | yes: TypeError when the running proc is not a C function or has no env; IndexError when idx is out of range (proc.c:389-406) | yes (raise path only) | idx: mrb_int, checked | Old name. The function is in mruby-proc-ext (proc.h:245). |
| MRB_METHOD_FUNC_FL | proc.h:251 | `(1 << 24)` | none | none | no | no | no argument | Bit of mrb_method_t.flags. |
| MRB_METHOD_PUBLIC_FL | proc.h:252 | 0 | none | none | no | no | no argument | |
| MRB_METHOD_PRIVATE_FL | proc.h:253 | `(1 << 25)` | none | none | no | no | no argument | |
| MRB_METHOD_PROTECTED_FL | proc.h:254 | `(1 << 26)` | none | none | no | no | no argument | |
| MRB_METHOD_VDEFAULT_FL | proc.h:255 | bits 25 and 26 | none | none | no | no | no argument | |
| MRB_METHOD_VISIBILITY_MASK | proc.h:256 | bits 25 and 26 | none | none | no | no | no argument | |
| MRB_METHOD_FUNC_P | proc.h:258 | `((m).flags & MRB_METHOD_FUNC_FL)` | none | none | no | no | m: an mrb_method_t (not a pointer) | |
| MRB_METHOD_FUNC | proc.h:259 | `((m).as.func)` | returns borrowed | none | no | no | m: mrb_method_t; meaningful only when MRB_METHOD_FUNC_P(m) | |
| MRB_METHOD_FROM_FUNC | proc.h:260 | sets m.flags to MRB_METHOD_FUNC_FL and m.as.func to fn | none | modifies m | no | no | m: mrb_method_t lvalue; fn: mrb_func_t | Statement macro. Clears visibility bits. |
| MRB_METHOD_FROM_PROC | proc.h:261 | sets m.flags to 0 and m.as.proc to pr | stores pr in m | modifies m | no | no | m: lvalue; pr: const struct RProc* | Statement macro. m does not keep pr alive; the method table that receives m does (gc.c:977-997 marks method tables). |
| MRB_METHOD_PROC_P | proc.h:262 | !MRB_METHOD_FUNC_P(m) | none | none | no | no | m: mrb_method_t | Also true for an undefined method (as.proc NULL). |
| MRB_METHOD_PROC | proc.h:263 | `((m).as.proc)` | returns borrowed | none | no | no | m: mrb_method_t | |
| MRB_METHOD_UNDEF_P | proc.h:264 | `((m).as.proc==NULL)` | none | none | no | no | m: mrb_method_t | |
| MRB_METHOD_NOTIMPL_P | proc.h:271 | a function method whose body is mrb_notimplement_m | none | none | no | no | m: mrb_method_t | |
| MRB_METHOD_VISIBILITY | proc.h:272 | `((m).flags & MRB_METHOD_VISIBILITY_MASK)` | none | none | no | no | m: mrb_method_t | |
| MRB_SET_VISIBILITY_FLAGS | proc.h:273 | `(f) = ((f) & ~MASK) \| (v)` | none | modifies f | no | no | f: lvalue; v: not masked | Evaluates f twice. |
| MRB_METHOD_SET_VISIBILITY | proc.h:274 | MRB_SET_VISIBILITY_FLAGS(m.flags, v) | none | modifies m | no | no | m: mrb_method_t lvalue | |
| MRB_METHOD_CFUNC_P | proc.h:276 | a function method, or a proc method whose proc is a C function | none | none | no | no | m: mrb_method_t | Evaluates m up to three times. |
| MRB_METHOD_CFUNC | proc.h:278 | as.func, or the C function of the proc | returns borrowed | none | no | no | m: mrb_method_t; only valid when MRB_METHOD_CFUNC_P(m) (proc.h:277) | |
| mrb_gc_free_range | range.h:28 (MRB_RANGE_EMBED); range.h:41 | `((void)0)`, or mrb_free(mrb, p->edges) | none | frees p->edges | no | no | p: struct RRange* | The GC calls it when it frees a Range (gc.c:1330-1332). A call from an extension frees memory the Range still uses. |
| RANGE_BEG | range.h:29; range.h:42 | `(p)->beg` or `(p)->edges->beg` | returns borrowed | none | no | no | p: struct RRange*; unchecked; edges is NULL until the Range is initialized | An lvalue; a write needs a write barrier and breaks the rule that a Range is immutable (range.c:87-89). |
| RANGE_END | range.h:30; range.h:43 | `(p)->end` or `(p)->edges->end` | returns borrowed | none | no | no | as RANGE_BEG | |
| mrb_range_beg | range.h:46 | RANGE_BEG(mrb_range_ptr(mrb, r)) | returns borrowed | none | yes: ArgumentError "uninitialized range" (range.c:491-493) | yes (raise path only) | r: a Range value; the type is unchecked (range.c:486-495 casts without a test) | |
| mrb_range_end | range.h:47 | RANGE_END(mrb_range_ptr(mrb, r)) | returns borrowed | none | yes: as mrb_range_beg | yes (raise path only) | as mrb_range_beg | |
| mrb_range_excl_p | range.h:48 | RANGE_EXCL(mrb_range_ptr(mrb, r)) | none | none | yes: as mrb_range_beg | yes (raise path only) | as mrb_range_beg | |
| mrb_range_raw_ptr | range.h:49 | `((struct RRange*)mrb_ptr(r))` | returns borrowed | returns pointer to the Range object | no | no | r: a Range value; unchecked | Skips the initialized check. |
| mrb_range_value | range.h:50 | mrb_obj_value((void*)(p)) | returns borrowed | none | no | no | p: struct RRange* | |
| RANGE_EXCL | range.h:51 | `((p)->excl)` | none | none | no | no | p: struct RRange* | |
| RANGE_INITIALIZED_FLAG | range.h:56 | 1 | none | none | no | no | no argument | Bit 0 of RRange.flags. |
| RANGE_INITIALIZED | range.h:57 | sets bit 0 of p->flags | none | modifies p->flags; no frozen check | no | no | p: struct RRange* | From now on the GC reads beg and end (range.c:466-471). Set it only after both hold valid values. |
| RANGE_INITIALIZED_P | range.h:58 | `((p)->flags & RANGE_INITIALIZED_FLAG)` | none | none | no | no | p: struct RRange* | |
| ISTRUCT_DATA_SIZE | istruct.h:20 | `(sizeof(void*) * 3)` | none | none | no | no | no argument | |
| RISTRUCT | istruct.h:30 | `((struct RIStruct*)(mrb_ptr(obj)))` | returns borrowed | returns pointer to the object | no | no | obj: an ISTRUCT value; unchecked | |
| ISTRUCT_PTR | istruct.h:31 | `(RISTRUCT(obj)->inline_data)` | returns borrowed | returns pointer into obj (inside the object); a write has no frozen check | no | no | obj: an ISTRUCT value; unchecked | The GC does not mark the bytes (gc.c:1194 default case) and frees nothing, so they must not hold an mrb_value or an owned pointer. |
| MRB_EXC_EXIT | error.h:26 | 65536 (bit 16 of RException.flags) | none | none | no | no | no argument | Set by mruby-exit (mrbgems/mruby-exit/src/mruby_exit.c:46). |
| MRB_EXC_EXIT_P | error.h:27 | `((e)->flags & MRB_EXC_EXIT)` | none | none | no | no | e: a pointer to the exception object | |
| MRB_EXC_EXIT_STATUS | error.h:29 | (int)mrb_as_int(mrb, mrb_obj_iv_get(mrb, e, MRB_IVSYM(status))) | none | none | yes: TypeError when @status is not numeric (nil gives TypeError, object.c:678) | yes | e: struct RObject* (mrb->exc has this type); needs mruby/variable.h and presym | The result is cut to int. |
| MRB_EXC_CHECK_EXIT | error.h:31 | calls exit() with MRB_EXC_EXIT_STATUS when the exit flag is set | none | none | yes: as MRB_EXC_EXIT_STATUS | yes | as MRB_EXC_EXIT_STATUS | Statement macro. Ends the process without mrb_close. |
| mrb_exc_ptr | error.h:33 | `((struct RException*)mrb_ptr(v))` | returns borrowed | returns pointer to the exception object | no | no | v: an exception value; unchecked | |
| mrb_exc_new_lit | error.h:37 | mrb_exc_new_str(mrb, c, mrb_str_new_lit(mrb, lit)) | returns new (arena) | none | yes: TypeError when c's instance type is not MRB_TT_EXCEPTION (gc.c:876-883); NoMemoryError | yes | c: struct RClass*; lit: a string literal | Does not raise the exception. |
| mrb_break_value_get | error.h:74 (64-bit or word boxing) | `((brk)->val)` | returns borrowed | none | no | no | brk: struct RBreak* | On other builds it is a static inline function (error.h:77-83, 91-98). |
| mrb_break_value_set | error.h:75 (64-bit or word boxing) | `((brk)->val = v)` | stores v in brk | modifies brk; no write barrier | no | no | brk: struct RBreak*; v: mrb_value | RBreak objects belong to the VM. |
| RBREAK_VALUE_TT_MASK | error.h:90 (32-bit no boxing) | `((1 << 8) - 1)` | none | none | no | no | no argument | Bits 0-7 of RBreak.flags hold the type of the stored value. |
| MRB_ENSURE | error.h:183 | a for statement: runs func(mrb, data) under mrb_protect_error, sets mrb->exc to the exception or NULL, runs the body once, then re-raises the exception when mrb->jmp is set | result_var holds new (arena) value; stores the exception in mrb->exc | none | yes: re-raises what func raised (runs Ruby code through func) | yes | result_var: mrb_value lvalue; func: mrb_protect_error_func*; data: void* | A `break` in the body skips the re-raise and leaves mrb->exc set. On success it clears any older mrb->exc. At the top level (mrb->jmp NULL) the exception stays in mrb->exc. |
| MRB_EACH_OBJ_OK | gc.h:17 | 0 | none | none | no | no | no argument | Return value of an mrb_each_object_callback: go on. |
| MRB_EACH_OBJ_BREAK | gc.h:18 | 1 | none | none | no | no | no argument | Return value: stop. |
| MRB_GC_RED | gc.h:168 | 7 | none | none | no | no | no argument | The gc_color of a static object that the GC never frees (gc.c:259-267; used at proc.c:40). |
| TYPED_POSFIXABLE | numeric.h:19 | `((f) <= (t)MRB_FIXNUM_MAX)` | none | none | no | no | f: number; t: a type | |
| TYPED_NEGFIXABLE | numeric.h:20 | `((f) >= (t)MRB_FIXNUM_MIN)` | none | none | no | no | f: number; t: a type | |
| TYPED_FIXABLE | numeric.h:21 | both of the above | none | none | no | no | f: number; t: a type | Evaluates f twice. |
| POSFIXABLE | numeric.h:22 | TYPED_POSFIXABLE(f, mrb_int) | none | none | no | no | f: integer | |
| NEGFIXABLE | numeric.h:23 | TYPED_NEGFIXABLE(f, mrb_int) | none | none | no | no | f: integer | |
| FIXABLE | numeric.h:24 | TYPED_FIXABLE(f, mrb_int) | none | none | no | no | f: integer | True when the number fits the value word without a heap object (MRB_FIXNUM_MIN..MAX). |
| FIXABLE_FLOAT | numeric.h:27 (MRB_INT64); numeric.h:29 | whether a float lies in the mrb_int range | none | none | no | no | f: floating value; a NaN gives false | Evaluates f twice. Not defined under MRB_NO_FLOAT. |
| mrb_num_plus | numeric.h:38 | mrb_num_add(mrb, x, y) | returns new (arena) for a Float, Bigint or heap Integer result | none | yes: TypeError "no number addition" when x is not numeric (numops.c:41); RangeError on overflow without mruby-bigint; TypeError from mrb_as_float for a Float x with a non-numeric y | yes | x, y: mrb_value; checked: raises TypeError | Obsolete name. |
| mrb_num_minus | numeric.h:39 | mrb_num_sub(mrb, x, y) | as mrb_num_plus | none | yes: as mrb_num_plus | yes | checked: raises TypeError | Obsolete name. |
| mrb_value_from_size_t | numeric.h:73 | mrb_uint64_value(mrb, (uint64_t)v) | returns new (arena) when the value needs a heap Integer or a Bigint | none | yes: RangeError when v does not fit mrb_int and mruby-bigint is not in (numeric.c:53-62); NoMemoryError | yes | v: an unsigned integer | |
| mrb_value_from_ssize_t | numeric.h:74 | mrb_int64_value(mrb, (int64_t)v) | as mrb_value_from_size_t | none | yes: as mrb_value_from_size_t (numeric.c:69-79) | yes | v: a signed integer | |
| mrb_fixnum_to_str | numeric.h:101 | mrb_integer_to_str(mrb, x, base) | returns new (arena) | none | yes: ArgumentError for a base outside 2..36 (numeric.c:2181-2183); NoMemoryError | yes | x: an Integer value; unchecked: a non-Integer is read with mrb_integer (numeric.c:2188) | Obsolete. |
| mrbc_context | compile.h:49 | the type name mrb_ccontext | none | none | no | no | no argument | Compatibility name. |
| mrbc_context_new | compile.h:50 | the function name mrb_ccontext_new | returns new (heap, not GC): the caller frees it with mrb_ccontext_free | none | yes: NoMemoryError (mrb_calloc, mrbgems/mruby-compiler/src/mruby_compat.c:331-334) | yes | mrb: mrb_state* | Object-like macro that renames a function. |
| mrbc_context_free | compile.h:51 | mrb_ccontext_free | none | frees the context, its syms and its filename (mruby_compat.c:337-343) | no | no | a NULL context is allowed | |
| mrbc_filename | compile.h:52 | mrb_ccontext_filename | none | copies s into the context (mruby_compat.c:346-359) and returns that copy | yes: NoMemoryError | yes | c may be NULL (gives NULL) | The returned pointer is valid until the next call or mrb_ccontext_free. |
| mrbc_partial_hook | compile.h:53 | mrb_ccontext_partial_hook | keeps pointer data in the context (mruby_compat.c:362-367) | none | no | no | c may be NULL | |
| mrbc_cleanup_local_variables | compile.h:54 | mrb_ccontext_cleanup_local_variables | none | resets slen and keep_lv (mruby_compat.c:370-380) | no | no | c may be NULL | |
| MRB_OPSYM | presym.h:41 | the token MRB_OPSYM__##name, a constant from the generated mruby/presym/id.h | none | none | no | no | name: an operator name token such as add or aref; a name not in the presym table does not compile | id.h is generated per build. In the build tree that is present (build/host/include/mruby/presym/id.h) each name is an enum constant under MRB_PRESYM_ENUM and a #define otherwise (id.h:1, 483-488). The set of names depends on the gems of the build: that tree has no MRB_SYM__FiberError. |
| MRB_GVSYM | presym.h:42 | MRB_GVSYM__##name, the symbol $name | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_CVSYM | presym.h:43 | MRB_CVSYM__##name, the symbol @@name | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_IVSYM | presym.h:44 | MRB_IVSYM__##name, the symbol @name | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_SYM_B | presym.h:45 | MRB_SYM_B__##name, the symbol name! | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_SYM_Q | presym.h:46 | MRB_SYM_Q__##name, the symbol name? | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_SYM_E | presym.h:47 | MRB_SYM_E__##name, the symbol name= | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_SYM | presym.h:48 | MRB_SYM__##name, the symbol name | none | none | no | no | name: identifier token | As MRB_OPSYM. |
| MRB_OPSYM_2 | presym.h:51 | MRB_OPSYM(name) | none | none | no | no | mrb: not evaluated | Backward-compatible form. |
| MRB_GVSYM_2 | presym.h:52 | MRB_GVSYM(name) | none | none | no | no | mrb: not evaluated | |
| MRB_CVSYM_2 | presym.h:53 | MRB_CVSYM(name) | none | none | no | no | mrb: not evaluated | |
| MRB_IVSYM_2 | presym.h:54 | MRB_IVSYM(name) | none | none | no | no | mrb: not evaluated | |
| MRB_SYM_B_2 | presym.h:55 | MRB_SYM_B(name) | none | none | no | no | mrb: not evaluated | |
| MRB_SYM_Q_2 | presym.h:56 | MRB_SYM_Q(name) | none | none | no | no | mrb: not evaluated | |
| MRB_SYM_E_2 | presym.h:57 | MRB_SYM_E(name) | none | none | no | no | mrb: not evaluated | |
| MRB_SYM_2 | presym.h:58 | MRB_SYM(name) | none | none | no | no | mrb: not evaluated | |
| MRB_PRESYM_DEFINE_VAR_AND_INITER | presym.h:60 | `static const mrb_sym name[] = {__VA_ARGS__};` | none | none | no | no | name: identifier; size: not used | A declaration, not an expression. |
| MRB_PRESYM_INIT_SYMBOLS | presym.h:63 | `(void)(mrb)` | none | none | no | no | name: not used | Does nothing outside presym scanning. |
| REGEXP_CLASS | re.h:12 | the string literal "Regexp" | none | none | no | no | no argument | A class name for lookup, for example with mrb_class_get. |
| MRB_TRY | throw.h:42 (MRB_USE_CXX_EXCEPTION); throw.h:68 | C++: `try {`. C: `if (MRB_SETJMP((buf)->impl) == 0) {` | none | none | no | no | buf: struct mrb_jmpbuf* | Opens a block that MRB_CATCH and MRB_END_EXC close. The caller must save mrb->jmp, point it at buf and restore it on both paths, as mrb_protect_error does (vm.c:1246-1285). Locals changed after setjmp need volatile in C. |
| MRB_CATCH | throw.h:43; throw.h:69 | C++: `} catch(mrb_jmpbuf *e) { if (e != (buf)) { throw e; }`. C: `} else {` | none | none | no | no | buf: the same struct mrb_jmpbuf* | In the catch branch the exception is in mrb->exc. |
| MRB_END_EXC | throw.h:44; throw.h:70 | `}` | none | none | no | no | buf: not used | |
| MRB_THROW | throw.h:46; throw.h:72 | C++: `throw(buf)`. C: `MRB_LONGJMP((buf)->impl, 1);` | none | none | yes: it is the raise mechanism itself | no | buf: struct mrb_jmpbuf* | The C form ends with a semicolon. Set mrb->exc before it, or the catcher reads a stale exception. mrb_exc_raise is the public way to raise. |

## Left out

The public headers define 760 distinct macro names (967 #define lines, counted with grep over include/mruby.h and include/mruby/*.h without internal.h). The table above has 450 rows. The other 310 names are below.

Include guards (32): MRB_THROW_H, MRUBY_ARRAY_H, MRUBY_BOXING_NAN_H, MRUBY_BOXING_NO_H, MRUBY_BOXING_WORD_H, MRUBY_CLASS_H, MRUBY_COMMON_H, MRUBY_COMPILE_H, MRUBY_DATA_H, MRUBY_DEBUG_H, MRUBY_DUMP_H, MRUBY_ENDIAN_H, MRUBY_ERROR_H, MRUBY_GC_H, MRUBY_H, MRUBY_HASH_H, MRUBY_IREP_H, MRUBY_ISTRUCT_H, MRUBY_KHASH_H, MRUBY_MEMPOOL_H, MRUBY_NUMERIC_H, MRUBY_OBJECT_H, MRUBY_OPCODE_H, MRUBY_PLATFORM_H, MRUBY_PRESYM_H, MRUBY_PROC_H, MRUBY_RANGE_H, MRUBY_RE_H, MRUBY_STRING_H, MRUBY_VALUE_H, MRUBY_VARIABLE_H, MRUBY_VERSION_H.

Configuration switches and the values they set (33): MRB_ARY_NO_EMBED, MRB_CONST_CACHE_SIZE, MRB_FIXED_STATE_ATEXIT_STACK_SIZE, MRB_FLOAT_EPSILON, MRB_FLT_DIG, MRB_FLT_EPSILON, MRB_FLT_MANT_DIG, MRB_FLT_MAX, MRB_FLT_MAX_10_EXP, MRB_FLT_MAX_EXP, MRB_FLT_MIN, MRB_FLT_MIN_10_EXP, MRB_FLT_MIN_EXP, MRB_FLT_RADIX, MRB_GC_ARENA_SIZE, MRB_GC_PROFILE_NBUCKETS, MRB_GRAY_STACK_SIZE, MRB_HAVE_TYPE_GENERIC_CHECKED_ARITHMETIC_BUILTINS, MRB_INT_BIT, MRB_INT_MAX, MRB_INT_MIN, MRB_IV_CACHE_SIZE, MRB_LINK_TIME_RO_DATA_P, MRB_METHOD_CACHE_SIZE, MRB_PARSER_TOKBUF_MAX, MRB_PARSER_TOKBUF_SIZE, MRB_PRId, MRB_PRIo, MRB_PRIx, MRB_RANGE_EMBED, MRB_SSIZE_MAX, MRB_USE_RBREAK_VALUE_UNION, MRB_WORDBOX_NO_INLINE_FLOAT.

Other names that do not act on values, objects or the state (245), by kind:

- Compiler, C library and platform shims (55): BIG_ENDIAN, BYTE_ORDER, DBL_EPSILON, FALSE, FLT_EPSILON, INFINITY, LDBL_EPSILON, LITTLE_ENDIAN, MRB_API, MRB_BEGIN_DECL, MRB_END_DECL, MRB_INLINE, MRB_LONGJMP, MRB_MEM_PREFETCH, MRB_MINGW32_LEGACY, MRB_MINGW32_VERSION, MRB_MINGW64_VERSION, MRB_SETJMP, MRUBY_PLATFORM, MRUBY_PLATFORM_CPU, MRUBY_PLATFORM_OS, NAN, PRId16, PRId32, PRId64, PRIo16, PRIo32, PRIo64, PRIu16, PRIu32, PRIu64, PRIx16, PRIx32, PRIx64, SIZE_MAX, TRUE, __STDC_CONSTANT_MACROS, __STDC_FORMAT_MACROS, __STDC_LIMIT_MACROS, __func__, __has_builtin, inline, isfinite, isinf, isnan, littleendian, mrb_alignas, mrb_deprecated, mrb_jmpbuf_impl, mrb_likely, mrb_noreturn, mrb_unlikely, signbit, snprintf, vsnprintf.
- Debug and static assertions (12): _mrb_static_assert_cat, _mrb_static_assert_cat0, _mrb_static_assert_id, mrb_assert, mrb_assert_int_fit, mrb_static_assert, mrb_static_assert1, mrb_static_assert2, mrb_static_assert_expand, mrb_static_assert_object_size, mrb_static_assert_powerof2, mrb_static_assert_selector.
- ASCII character classes on a C char (13): ISALNUM, ISALPHA, ISASCII, ISBLANK, ISCNTRL, ISDIGIT, ISLOWER, ISPRINT, ISSPACE, ISUPPER, ISXDIGIT, TOLOWER, TOUPPER.
- Windows locale conversion of C strings, no-ops elsewhere (4): mrb_locale_free, mrb_locale_from_utf8, mrb_utf8_free, mrb_utf8_from_locale.
- Version and release strings (24): MRB_STRINGIZE, MRB_STRINGIZE0, MRUBY_AUTHOR, MRUBY_BIRTH_YEAR, MRUBY_COPYRIGHT, MRUBY_DESCRIPTION, MRUBY_FULL_REVISION, MRUBY_PATCHLEVEL, MRUBY_PATCHLEVEL_STR, MRUBY_RELEASE_DATE, MRUBY_RELEASE_DAY, MRUBY_RELEASE_DAY_STR, MRUBY_RELEASE_MAJOR, MRUBY_RELEASE_MINOR, MRUBY_RELEASE_MONTH, MRUBY_RELEASE_MONTH_STR, MRUBY_RELEASE_NO, MRUBY_RELEASE_TEENY, MRUBY_RELEASE_YEAR, MRUBY_RELEASE_YEAR_STR, MRUBY_REVISION, MRUBY_RUBY_ENGINE, MRUBY_RUBY_VERSION, MRUBY_VERSION.
- Bytecode, irep and RITE binary format, from opcode.h, irep.h and dump.h (81): FETCH_B, FETCH_BB, FETCH_BBB, FETCH_BBB_1, FETCH_BBB_2, FETCH_BBB_3, FETCH_BB_1, FETCH_BB_2, FETCH_BB_3, FETCH_BS, FETCH_BSS, FETCH_BSS_1, FETCH_BSS_2, FETCH_BSS_3, FETCH_BS_1, FETCH_BS_2, FETCH_BS_3, FETCH_B_1, FETCH_B_2, FETCH_B_3, FETCH_S, FETCH_S_1, FETCH_S_2, FETCH_S_3, FETCH_W, FETCH_W_1, FETCH_W_2, FETCH_W_3, FETCH_Z, FETCH_Z_1, FETCH_Z_2, FETCH_Z_3, IREP_TT_NFLAG, IREP_TT_SFLAG, MRB_DUMP_ALIGNMENT, MRB_DUMP_DEBUG_INFO, MRB_DUMP_DEFAULT_STR_LEN, MRB_DUMP_FLOAT_SIZE, MRB_DUMP_GENERAL_FAILURE, MRB_DUMP_INVALID_ARGUMENT, MRB_DUMP_INVALID_FILE_HEADER, MRB_DUMP_INVALID_IREP, MRB_DUMP_NO_LVAR, MRB_DUMP_NULL_SYM_LEN, MRB_DUMP_OK, MRB_DUMP_READ_FAULT, MRB_DUMP_STATIC, MRB_DUMP_WRITE_FAULT, MRB_IREP_CONSOLIDATED, MRB_IREP_NO_FREE, MRB_IREP_STATIC, MRB_ISEQ_NO_FREE, OPCODE, OP_LOADF, OP_LOADT, OP_L_BLOCK, OP_L_CAPTURE, OP_L_LAMBDA, OP_L_METHOD, OP_L_STRICT, PEEK_B, PEEK_S, PEEK_W, READ_B, READ_S, READ_W, RITE_BINARY_EOF, RITE_BINARY_FORMAT_VER, RITE_BINARY_IDENT, RITE_BINARY_MAJOR_VER, RITE_BINARY_MINOR_VER, RITE_COMPILER_NAME, RITE_COMPILER_VERSION, RITE_LV_NULL_MARK, RITE_SECTION_DEBUG_IDENT, RITE_SECTION_HEADER, RITE_SECTION_IREP_IDENT, RITE_SECTION_LV_IDENT, RITE_VM_VER, mrb_irep_catch_handler_pack, mrb_irep_catch_handler_unpack.
- The khash C container, khash.h (44): KHASH_CHECK_MODIFIED, KHASH_DECLARE, KHASH_DEFINE, KHASH_FOREACH, KHASH_INITIAL_SIZE, KHASH_MIN_SIZE, KHASH_SMALL_LIMIT, KH_UPPER_BOUND, __ac_isdel, __ac_iseither, __ac_isempty, kh_begin, kh_clear, kh_copy, kh_del, kh_destroy, kh_destroy_data, kh_end, kh_exist, kh_get, kh_init, kh_init_data, kh_init_size, kh_int64_hash_equal, kh_int64_hash_func, kh_int_hash_equal, kh_int_hash_func, kh_is_end, kh_key, kh_n_buckets, kh_put, kh_put2, kh_replace, kh_resize, kh_size, kh_str_hash_equal, kh_str_hash_func, kh_val, kh_value, khash_mask, khash_power2, khash_t, khash_upper_bound, mrb_int_hash_func.
- The parser memory pool, mempool.h (4): mrb_mempool_alloc, mrb_mempool_close, mrb_mempool_open, mrb_mempool_realloc.
- Unique-name helpers for other macros (3): MRB_UNIQNAME, MRB_UNIQNAME_1, MRB_UNIQNAME_2.
- Undefined again in the same header, so not usable after it (3): MRB_INT_OVERFLOW_MASK, MRB_VTYPE_DEFINE, MRB_VTYPE_TYPEDEF (numeric.h:188, value.h:226, value.h:237).
- Inside an `#if 0` block, so never defined (2): memcpy, memset (mruby.h:1842-1867).

include/mruby/presym/scanning.h is under presym/ and is left out by the scope rule. It redefines mrb_intern_lit, MRB_SYM and other names only while MRB_PRESYM_SCANNING is defined.
