Extra Clang Tools 9.0.0 (In-Progress) Release Notes
===================================================
>>>>>>> official_clang_tidy/master

.. contents::
   :local:
   :depth: 3

Written by the `LLVM Team <https://llvm.org/>`_

Introduction
============

This document contains the release notes for the Extra Clang Tools, part of the
Clang release 9.0.0. Here we describe the status of the Extra Clang Tools in
some detail, including major improvements from the previous release and new
feature work. All LLVM releases may be downloaded from the `LLVM releases web
site <https://llvm.org/releases/>`_.

For more information about Clang or LLVM, including information about
the latest release, please see the `Clang Web Site <https://clang.llvm.org>`_ or
the `LLVM Web Site <https://llvm.org>`_.

<<<<<<< HEAD
What's New in Extra Clang Tools 3.9?
====================================
=======
Note that if you are reading this file from a Subversion checkout or the
main Clang web page, this document applies to the *next* release, not
the current one. To see the release notes for a specific release, please
see the `releases page <https://llvm.org/releases/>`_.

What's New in Extra Clang Tools 9.0.0?
======================================
>>>>>>> official_clang_tidy/master

Some of the major new features and improvements to Extra Clang Tools are listed
here. Generic improvements to Extra Clang Tools as a whole or to its underlying
infrastructure are described first, followed by tool-specific sections.

Major New Features
------------------

...

Improvements to clangd
----------------------

The improvements are...

Improvements to clang-doc
-------------------------

The improvements are...

<<<<<<< HEAD
  It aims to provide automated insertion of missing ``#includes`` with a single
  button press in an editor. Integration with Vim and a tool to generate the
  symbol index used by the tool are also part of this release. See the
  `include-fixer documentation`_ for more information.
=======
Improvements to clang-query
---------------------------

- ...
>>>>>>> official_clang_tidy/master

.. _include-fixer documentation: http://clang.llvm.org/extra/include-fixer.html

Improvements to clang-tidy
--------------------------

- New :doc:`abseil-duration-addition
  <clang-tidy/checks/abseil-duration-addition>` check.

  Checks for cases where addition should be performed in the ``absl::Time``
  domain.

- New :doc:`abseil-duration-conversion-cast
  <clang-tidy/checks/abseil-duration-conversion-cast>` check.

  Checks for casts of ``absl::Duration`` conversion functions, and recommends
  the right conversion function instead.

- New :doc:`abseil-duration-unnecessary-conversion
  <clang-tidy/checks/abseil-duration-unnecessary-conversion>` check.

  Finds and fixes cases where ``absl::Duration`` values are being converted to
  numeric types and back again.

- New :doc:`google-readability-avoid-underscore-in-googletest-name
  <clang-tidy/checks/google-readability-avoid-underscore-in-googletest-name>`
  check.

  Checks whether there are underscores in googletest test and test case names in
  test macros, which is prohibited by the Googletest FAQ.

- The :doc:`bugprone-argument-comment
  <clang-tidy/checks/bugprone-argument-comment>` now supports
  `CommentBoolLiterals`, `CommentIntegerLiterals`,  `CommentFloatLiterals`,
  `CommentUserDefiniedLiterals`, `CommentStringLiterals`,
  `CommentCharacterLiterals` & `CommentNullPtrs` options.

Improvements to include-fixer
-----------------------------

<<<<<<< HEAD
  Does not only checks for correct signature but also for correct ``return``
  statements (returning ``*this``)

- New `misc-unused-using-decls
  <http://clang.llvm.org/extra/clang-tidy/checks/misc-unused-using-decls.html>`_ check

  Finds unused ``using`` declarations.

- New `modernize-avoid-bind
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-avoid-bind.html>`_ check

  Finds uses of ``std::bind`` and replaces simple uses with lambdas.

- New `modernize-deprecated-headers
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-deprecated-headers.html>`_ check

  Replaces C standard library headers with their C++ alternatives.

- New `modernize-make-shared
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-make-shared.html>`_ check

  Replaces creation of ``std::shared_ptr`` from new expression with call to ``std::make_shared``.

- New `modernize-raw-string-literal
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-raw-string-literal.html>`_ check

  Selectively replaces string literals containing escaped characters with raw
  string literals.

- New `modernize-use-bool-literals
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-use-bool-literals.html>`_ check

  Finds integer literals which are cast to ``bool``.

- New `modernize-use-emplace
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-use-emplace.html>`_ check

  Finds calls that could be changed to emplace.

- New `modernize-use-using
  <http://clang.llvm.org/extra/clang-tidy/checks/modernize-use-using.html>`_ check

  Finds typedefs and replaces it with usings.

- New `performance-faster-string-find
  <http://clang.llvm.org/extra/clang-tidy/checks/performance-faster-string-find.html>`_ check

  Optimize calls to ``std::string::find()`` and friends when the needle passed
  is a single character string literal.

- New `performance-implicit-cast-in-loop
  <http://clang.llvm.org/extra/clang-tidy/checks/performance-implicit-cast-in-loop.html>`_ check

  Warns about range-based loop with a loop variable of const ref type where the
  type of the variable does not match the one returned by the iterator.

- New `performance-unnecessary-value-param
  <http://clang.llvm.org/extra/clang-tidy/checks/performance-unnecessary-value-param.html>`_ check

  Flags value parameter declarations of expensive to copy types that are copied
  for each invocation but it would suffice to pass them by const reference.

- New `readability-avoid-const-params-in-decls
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-avoid-const-params-in-decls.html>`_ check

  Warns about top-level const parameters in function declarations.

- New `readability-deleted-default
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-deleted-default.html>`_ check

  Warns about defaulted constructors and assignment operators that are actually
  deleted.

- Updated `readability-identifier-naming-check
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-identifier-naming.html>`_

  Added support for enforcing the case of macro statements.

- New `readability-redundant-control-flow
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-redundant-control-flow.html>`_ check

  Looks for procedures (functions returning no value) with ``return`` statements
  at the end of the function.  Such `return` statements are redundant.

- New `readability-redundant-string-init
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-redundant-string-init.html>`_ check

  Finds unnecessary string initializations.

- New `readability-static-definition-in-anonymous-namespace
  <http://clang.llvm.org/extra/clang-tidy/checks/readability-static-definition-in-anonymous-namespace.html>`_ check

  Finds static function and variable definitions in anonymous namespace.

Fixed bugs:

- Crash when running on compile database with relative source files paths.

- Crash when running with the `-fdelayed-template-parsing` flag.

- The `modernize-use-override` check: incorrect fix-its placement around
  ``__declspec`` and other attributes.

Clang-tidy changes from 3.7 to 3.8
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The 3.8 release didn't include release notes for :program:`clang-tidy`. In the
3.8 release many new checks have been added to :program:`clang-tidy`:

- Checks enforcing certain rules of the `CERT Secure Coding Standards
  <https://www.securecoding.cert.org/confluence/display/seccode/SEI+CERT+Coding+Standards>`_:

  * `cert-dcl03-c
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-dcl03-c.html>`_
    (an alias to the pre-existing check `misc-static-assert
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/misc-static-assert.html>`_)
  * `cert-dcl50-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-dcl50-cpp.html>`_
  * `cert-err52-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-err52-cpp.html>`_
  * `cert-err58-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-err58-cpp.html>`_
  * `cert-err60-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-err60-cpp.html>`_
  * `cert-err61-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-err61-cpp.html>`_
  * `cert-fio38-c
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-fio38-c.html>`_
    (an alias to the pre-existing check `misc-non-copyable-objects
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/misc-non-copyable-objects.html>`_)
  * `cert-oop11-cpp
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cert-oop11-cpp.html>`_
    (an alias to the pre-existing check `misc-move-constructor-init
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/misc-move-constructor-init.html>`_)

- Checks supporting the `C++ Core Guidelines
  <https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md>`_:

  * `cppcoreguidelines-pro-bounds-array-to-pointer-decay
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-bounds-array-to-pointer-decay.html>`_
  * `cppcoreguidelines-pro-bounds-constant-array-index
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-bounds-constant-array-index.html>`_
  * `cppcoreguidelines-pro-bounds-pointer-arithmetic
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-bounds-pointer-arithmetic.html>`_
  * `cppcoreguidelines-pro-type-const-cast
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-const-cast.html>`_
  * `cppcoreguidelines-pro-type-cstyle-cast
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-cstyle-cast.html>`_
  * `cppcoreguidelines-pro-type-reinterpret-cast
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-reinterpret-cast.html>`_
  * `cppcoreguidelines-pro-type-static-cast-downcast
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-static-cast-downcast.html>`_
  * `cppcoreguidelines-pro-type-union-access
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-union-access.html>`_
  * `cppcoreguidelines-pro-type-vararg
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-pro-type-vararg.html>`_

- The functionality of the :program:`clang-modernize` tool has been moved to the
  new ``modernize`` module in :program:`clang-tidy` along with a few new checks:

  * `modernize-loop-convert
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-loop-convert.html>`_
  * `modernize-make-unique
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-make-unique.html>`_
  * `modernize-pass-by-value
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-pass-by-value.html>`_
  * `modernize-redundant-void-arg
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-redundant-void-arg.html>`_
  * `modernize-replace-auto-ptr
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-replace-auto-ptr.html>`_
  * `modernize-shrink-to-fit
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-shrink-to-fit.html>`_
    (renamed from ``readability-shrink-to-fit``)
  * `modernize-use-auto
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-use-auto.html>`_
  * `modernize-use-default
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-use-default.html>`_
  * `modernize-use-nullptr
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-use-nullptr.html>`_
  * `modernize-use-override
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/modernize-use-override.html>`_
    (renamed from ``misc-use-override``)

- New checks flagging various readability-related issues:

  * `readability-identifier-naming
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/readability-identifier-naming.html>`_
  * `readability-implicit-bool-cast
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/readability-implicit-bool-cast.html>`_
  * `readability-inconsistent-declaration-parameter-name
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/readability-inconsistent-declaration-parameter-name.html>`_
  * `readability-uniqueptr-delete-release
    <http://llvm.org/releases/3.8.0/tools/clang/tools/extra/docs/clang-tidy/checks/readability-uniqueptr-delete-release.html>`_

- Updated ``cppcoreguidelines-pro-member-type-member-init`` check

  This check now conforms to C++ Core Guidelines rule Type.6: Always Initialize
  a Member Variable. The check examines every record type where construction
  might result in an undefined memory state. These record types needing
  initialization have at least one default-initialized built-in, pointer,
  array or record type matching these criteria or a default-initialized
  direct base class of this kind.

  The check has two complementary aspects:

  1. Ensure every constructor for a record type needing initialization
     value-initializes all members and direct bases via a combination of
     in-class initializers and the member initializer list.
  2. Value-initialize every non-member instance of a record type needing
     initialization that lacks a user-provided default constructor, e.g.
     a POD.
=======
The improvements are...

Improvements to modularize
--------------------------

The improvements are...
>>>>>>> official_clang_tidy/master
