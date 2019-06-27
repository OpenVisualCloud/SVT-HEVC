
# Code Style


Tabs vs Spaces\
**No tabs,** only spaces; 4-space indentation

Be aware that some tools might add tabs when auto aligning the code, please check your commits with a diff tool for tabs.

for multi-line statements, the indentation of the next line depends on the context of the statement and braces around it.\
For example, if you have a long assignment, you can choose to either align it to the = of the first line, or (if that leads to less lines of code) just indent 1 level further from the first line's indentation level:

``` c
const int my_var = something1 &&
                   something2;
```

or

``` c
const int my_var = something1 +
    something2 - something3 * something4;
```

However, if there are braces, the first non-whitespace character of the line should be aligned with the brace level that it is part of:

``` c
const int my_var = (something1 +
                    something2) * something3;
```

Use `CamelCase` for types and `under_score` for variable names (`TypeName my_instance;`)\
Use const where possible, except in forward function declarations in header files, where we only use it for const-arrays:

``` c
int my_func(const array *values, int arg);

[..]

int my_func(const array *const values, const int num) {
    [..]
}
```

Braces go on the same line for single-line statements, but on a new line for multi-line statements:

``` c
static void function(const int argument) {
    do_something();
}
```

versus

``` c
static void function(const int argument1,
                     const int argument2)
{
    do_something();
}
```

Braces are only necessary for multi-line code blocks or multi-line condition statements;

``` c
if (condition1 && condition2)
    do_something();
```

and

``` c
if (condition) {
    do_something_1();
    do_something_2();
}
```

and

``` c
if (condition1 &&
    condition2)
{
    do_something();
}
```

Switch/case are indented at the same level, and the code block is indented one level deeper:

``` c
switch (a) {
case 1:
    bla();
    break;
}
```

but for very trivial blocks, you can also put everything on one single line:

``` c
switch (a) {
case 1: bla(); break;
}
```

Lines should idealy not be longer than 80 characters. We allow exceptions if wrapping the line would lead to exceptional ugliness, and this is done on a case-by-case basis.\
Don't use `goto` except for standard error handling.\
Use native types (`int`, `unsigned`, etc.) for scalar variables where the upper bound of a size doesn't matter.\
Use sized types (`uint8_t`, `int16_t`, etc.) for vector/array variables where the upper bound of the size matters.\
Use dynamic types (`pixel`, `coef`, etc.) so multi-bitdepth templating works as it should.

## Doxygen Documentation

``` c
/* File level Description */

/*********************************************************************************
* @file
*  file.c
*
* @brief
*  Brief description about file
*
* @author
*  Author
*
* @par List of Functions:
* - fun1()
* - fun2()
*
* @remarks
*  Any remarks
*
********************************************************************************/

/* Macro Description */
/** Brief description of MACRO */
#define MACRO   val

/* enum Description : description for all entries */
/** Brief description of ENUMs */
enum {
    ENUM1         = 1, /**< Brief description of ENUM1 */
    ENUM2         = 2, /**< Brief description of ENUM2 */
    ENUM3         = 3  /**< Brief description of ENUM3 */
}

/* enum Description : top level description */
/** Brief description of ENUMs */
enum {
    ENUM1         = 1,
    ENUM2         = 2,
    ENUM3         = 3
}

/* structure level Description */

struct {
    member1, /**< Brief description of member1 */
    member2, /**< Brief description of member2 */
    member3, /**< Brief description of member3 */
}

/* Function level Description */

/*********************************************************************************
*
* @brief
*  Brief description of function
*
* @par Description:
*  Detailed description of function
*
* @param[in] prm1
*  Brief description of prm1
*
* @param[in] prm2
*  Brief description of prm2
*
* @param[out] prm3
*  Brief description of prm3
*
* @returns  Brief description of return value
*
* @remarks
*  Any remarks
*
********************************************************************************/
```

## Post-coding

After coding, make sure to trim any trailing white space and convert any tabs to 4 spaces

### For bash

``` bash
find . -name <Filename> -type f -exec sed -i 's/\t/    /;s/[[:space:]]*$//' {} +
```

Where `<Filename>` is `"*.c"` or `"*.(your file extention here)"`\
Search the `find` man page or tips and tricks for more options.\
**Do not** use find on root without a filter or with the `.git` folder still present. Doing so will corrupt your repo folder and you will need to copy a new `.git` folder and re-setup your folder.

Alternatively, for single file(s):

``` bash
sed -i 's/\t/    /;s/[[:space:]]*$//' <Filename/Filepath>
```

Note: For macOS and BSD related distros, you may need to use `sed -i ''` inplace due to differences with GNU sed.

### For Powershell/pwsh

``` Powershell
ls -Recurse -File -Filter *.c | ForEach-Object{$(Get-Content $_.FullName | Foreach {Write-Output "$($_.TrimEnd().Replace("`t","    "))`n"}) | Set-Content -NoNewline -Encoding utf8 $_.FullName}
```

Where `-Filter *.c` has your extention/filename(s).\
This does not work with `pwsh` on non-windows OS.\
Search the docs for [`pwsh`](https://docs.microsoft.com/en-us/powershell/scripting/overview?view=powershell-6) related commands and [`powershell`](https://docs.microsoft.com/en-us/powershell/scripting/overview?view=powershell-5.1) related commands for more information on what they do.\
**Do not** use ls without a `-Filter` on the root directory or with the `.git` folder still present. Doing so will corrupt your repo folder and you will need to copy a new `.git` folder and re-setup your folder.

Alternatively, for a single file:

``` Powershell
$filename="<filename>"; Get-content $filename | Foreach {Write-Output "$($_.TrimEnd().Replace("`t","    "))`n"}) | Set-Content -NoNewline $filename
```

Where `<filename>` is the specific file you want to trim.
