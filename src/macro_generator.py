'''Generate macros for CTest.'''

one_variable = {
    "true": "!({value})",
    "false": "{value}",
}

two_variable = {
    "eq": "{a} != {b}",
    "ne": "{a} == {b}",
    "lt": "{a} >= {b}",
    "le": "{a} > {b}",
    "gt": "{a} <= {b}",
    "ge": "{a} < {b}",

    "streq": "strcmp({a}, {b}) != 0",
    "strne": "strcmp({a}, {b}) == 0",
    "strcaseq": "strcasecmp({a}, {b}) != 0",
    "strcasne": "strcasecmp({a}, {b}) == 0",
}

def enclose_variable(variable):
    '''Used for making macro variables safe.'''
    return '({})'.format(variable)

for macro_prefix, error_type in [('ASSERT', 'failure'), ('EXPECT', 'error')]:
    for name, template in one_variable.items():
        variable_name = 'a'
        macro_name = '{macro_prefix}_{name}'.format(macro_prefix=macro_prefix, name=name.upper())
        macro_cmp = template.format(value=enclose_variable(variable_name))
        macro_body = 'if ({macro_cmp}) {{ {return_fn} __runner->{error_type}(__runner,  "{macro_name}("#{variable_name}")", __FILE__, __LINE__, __VA_ARGS__); }}'.format(
            return_fn='return' if error_type == 'failure' else '',
            variable_name=variable_name,
            macro_name=macro_name,
            macro_cmp=macro_cmp,
            error_type=error_type,
        )
        macro = '#define {macro_name}({variable_name}, ...) {macro_body}'.format(
            macro_name=macro_name,
            variable_name=variable_name,
            macro_body=macro_body,
        )
        print macro

    print ''

    for name, template in two_variable.items():
        variable_names = ['a', 'b']
        macro_name = '{macro_prefix}_{name}'.format(macro_prefix=macro_prefix, name=name.upper())
        macro_cmp = template.format(a=enclose_variable(variable_names[0]), b=enclose_variable(variable_names[1]))
        macro_body = 'if ({macro_cmp}) {{ {return_fn} __runner->{error_type}(__runner, "{macro_name}("#{a}", "#{b}")", __FILE__, __LINE__, __VA_ARGS__); }}'.format(
            return_fn='return' if error_type == 'failure' else '',
            a=variable_names[0],
            b=variable_names[1],
            macro_name=macro_name,
            macro_cmp=macro_cmp,
            error_type=error_type,
        )
        macro = '#define {macro_name}({a}, {b}, ...) {macro_body}'.format(
            macro_name=macro_name,
            a=variable_names[0],
            b=variable_names[1],
            macro_body=macro_body,
        )
        print macro
    print ''
