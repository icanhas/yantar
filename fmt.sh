whitelist="asm botlib cgame client game q3_ui qcommon server renderergl2 renderer
sys sys/sdl cmd/asm ui"

opt= << "END"
align_assign_thresh = 2
align_assign_span = 1
align_on_tabstop = true # align on tabstops
align_right_cmt_thresh = 1
align_right_cmt_gap = 1
align_right_cmt_span = 2
align_right_cmt_at_col = 1
align_single_line_func = true
align_var_def_thresh 2
align_var_def_star_style = 1
align_var_def_gap = 1
align_var_def_span = 2
align_var_def_attribute = false
align_var_struct_thresh = 16
align_var_struct_span = 2
align_var_struct_gap = 1
align_func_params = false
align_pp_define_gap = 1
align_pp_define_span = 2
align_with_tabs = true # use tabs to align
cmt_c_nl_start = true
cmt_c_nl_end = true
cmt_c_group = false
cmt_cpp_group = true
cmt_cpp_to_c = true
cmt_star_cont = true
cmt_reflow_mode = 1
code_width = 110
indent_align_assign = true
indent_align_string = true
indent_columns = output_tab_size
indent_func_call_param = true
indent_label = 1 # pos: absolute col, neg: relative column
indent_with_tabs = 2 # 1=indent to level only, 2=indent with tabs
indent_cmt_with_tabs = true
input_tab_size = 8 # original tab size
mod_full_brace_for = remove
mod_full_brace_if = remove
mod_full_brace_while = remove
nl_after_brace_close = false
nl_before_case = false
nl_brace_else = remove # "} else" vs "} \n else" - cuddle else= add
nl_brace_while = remove # "} while" vs "} \n while" - cuddle while
nl_collapse_empty_body = false
nl_create_if_one_liner = false
nl_create_for_one_liner = false
nl_do_brace = remove # "do {" vs "do \n {"
nl_else_brace = remove # "else {" vs "else \n {"
nl_end_of_file = force
nl_end_of_file_min = 1
nl_enum_brace = remove # "enum {" vs "enum \n {"
nl_fdef_brace = add
nl_for_brace = remove # "for () {" vs "for () \n {"
nl_func_proto_type_name = remove
nl_func_type_name = add
nl_if_brace = remove # "if () {" vs "if () \n {"
nl_if_leave_one_liners = false
nl_struct_brace = remove # "struct {" vs "struct \n {"
nl_switch_brace = remove # "switch () {" vs "switch () \n {"
nl_union_brace = remove # "union {" vs "union \n {"
nl_while_brace = remove # "while () {" vs "while () \n {"
output_tab_size = 8 # new tab size
sp_after_cast = remove # "(int) a" vs "(int)a"
sp_cpp_cast_paren = remove
sp_after_sparen = remove # "if () {" vs "if (){"
sp_before_sparen = remove # "if (" vs "if("
sp_before_ptr_star_func = remove # star alignment before func defs/protos
sp_after_ptr_star_func = remove # "
sp_brace_else = remove
sp_else_brace = remove
sp_func_def_paren = remove
sp_func_proto_paren = remove
sp_inside_fparen = remove
sp_inside_paren = remove
sp_inside_sparen = remove
sp_inside_sparen_close = remove
sp_paren_brace = remove
sp_paren_paren = remove
sp_sizeof_paren = remove # "sizeof (int)" vs "sizeof(int)"
sp_type_func = ignore # break between return type and func name
END

echo $opt > .uncrustify

for d in $whitelist; do
	d="src/$d"
	if [ ! -d $d ]; then
		echo "$d is missing. I can't believe you've done this."
		exit 1
	fi

	filt="$d/*.c $d/*.h"
	for f in $filt; do
		cat $f | uncrustify -q -c .uncrustify > "$f.tmp"
		mv "$f.tmp" $f
	done
	echo -n .
done
echo

rm .uncrustify

