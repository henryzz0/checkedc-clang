//=--3CStandalone.cpp---------------------------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// 3C tool
//
//===----------------------------------------------------------------------===//

#include "clang/3C/3C.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;
// See clang/docs/checkedc/3C/clang-tidy.md#_3c-name-prefix
// NOLINTNEXTLINE(readability-identifier-naming)
static cl::OptionCategory _3CCategory("3C options");
static const char *HelpOverview =
    "3c: Automatically infer Checked C annotations for an existing C program "
    "(or one already partially converted to Checked C)\n";
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// Use a raw string to reduce the clutter of escape sequences and make it easier
// to line-wrap the message using a text editor. We actually want the final
// blank line in the message; the initial one just helps the editor know what
// text it is supposed to wrap.
//
// XXX: The first two paragraphs are common to all Clang LibTooling-based
// tools and would ideally go in CommonOptionsParser::HelpMessage or
// somewhere else that users will find. But unless/until we pursue that, we
// document that information here for 3c.
static const char MoreHelpStr[] = R"(

By default, 3c (like any Clang LibTooling-based tool) automatically searches for
a compilation database based on the -p option or (if that option is not
specified) the path of the first source file. If no compilation database is
found, 3c prints a warning. If one is found, 3c looks up each source file
specified on the command line in the compilation database to find the compiler
options to use for that file. Thus, if you give 3c a compilation database
generated by your build system, it will use the same compiler options as your
build system (which may be different for each file). You can add options via
-extra-arg and -extra-arg-before. If you want to run 3c on all source files in
your compilation database, you must pass them on the command line; 3c will not
take the list automatically from the compilation database. If you specify a
source file that is not in the database, 3c will use the compiler options from
the most "similar looking" file in the database according to a set of
heuristics.

If you _do not_ want to use a compilation database, pass "--" after all other 3c
arguments. This is important to ensure that 3c doesn't automatically detect a
compilation database and use compiler options you do not want from a "similar
looking" file in the database. The "--" may be followed by compiler options that
you want to use for all source files (this is equivalent to specifying those
options via -extra-arg before the "--").

You can use either -output-dir or -output-postfix to control the paths at which
3c writes the new versions of your files. With either of these options, if 3c
does not write a new version of a given file, that means the file needs no
changes. If you use neither -output-dir nor -output-postfix, then you can only
pass one source file on the command line and the new version of that file is
written to stdout regardless of whether it differs from the original ("stdout
mode"), but 3c still solves for changes to all files under the -base-dir that
are "#include"-d by that file and it is an error if any other file changes.

)";

// Skip the 2 initial newlines.
static cl::extrahelp MoreHelp(MoreHelpStr + 2);

static cl::opt<bool> OptDumpIntermediate("dump-intermediate",
                                         cl::desc("Dump intermediate "
                                                  "information"),
                                         cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptVerbose("verbose",
                                cl::desc("Print verbose "
                                         "information"),
                                cl::init(false), cl::cat(_3CCategory));

static cl::opt<std::string> OptOutputPostfix(
    "output-postfix",
    cl::desc("String to insert into the names of updated files just before the "
             "extension (e.g., with -output-postfix=checked, foo.c -> "
             "foo.checked.c)"),
    cl::init("-"), cl::cat(_3CCategory));

static cl::opt<std::string> OptOutputDir(
    "output-dir",
    cl::desc("Directory under which updated files will be written at the same "
             "relative paths as the originals under the -base-dir"),
    cl::init(""), cl::cat(_3CCategory));

static cl::opt<std::string>
    OptMalloc("use-malloc",
              cl::desc("Allows for the usage of user-specified "
                       "versions of function allocators"),
              cl::init(""), cl::cat(_3CCategory));

static cl::opt<std::string>
    OptConstraintOutputJson("constraint-output",
                            cl::desc("Path to the file where all the analysis "
                                     "information will be dumped as json"),
                            cl::init("constraint_output.json"),
                            cl::cat(_3CCategory));

static cl::opt<std::string>
    OptStatsOutputJson("stats-output",
                       cl::desc("Path to the file where all the stats "
                                "will be dumped as json"),
                       cl::init("TotalConstraintStats.json"),
                       cl::cat(_3CCategory));
static cl::opt<std::string>
    OptWildPtrInfoJson("wildptrstats-output",
                       cl::desc("Path to the file where all the info "
                                "related to WILD ptr grouped by reason"
                                " will be dumped as json"),
                       cl::init("WildPtrStats.json"), cl::cat(_3CCategory));

static cl::opt<std::string> OptPerPtrWILDInfoJson(
    "perptrstats-output",
    cl::desc("Path to the file where all the info "
             "related to each WILD ptr will be dumped as json"),
    cl::init("PerWildPtrStats.json"), cl::cat(_3CCategory));

static cl::opt<bool> OptDumpStats("dump-stats", cl::desc("Dump statistics"),
                                  cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptHandleVARARGS("handle-varargs",
                                      cl::desc("Enable handling of varargs "
                                               "in a "
                                               "sound manner"),
                                      cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool>
    OptEnablePropThruIType("enable-itypeprop",
                           cl::desc("Enable propagation of "
                                    "constraints through ityped "
                                    "parameters/returns."),
                           cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptAllTypes("alltypes",
                                 cl::desc("Consider all Checked C types for "
                                          "conversion"),
                                 cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptAddCheckedRegions("addcr",
                                          cl::desc("Add Checked "
                                                   "Regions"),
                                          cl::init(false),
                                          cl::cat(_3CCategory));

static cl::opt<bool>
    OptDiableCCTypeChecker("disccty",
                           cl::desc("Do not disable checked c type checker."),
                           cl::init(false), cl::cat(_3CCategory));

static cl::opt<std::string> OptBaseDir(
    "base-dir",
    cl::desc(
        "Ancestor directory defining the set of files that 3c "
        "is allowed to modify (default: the working "
        "directory). All source files specified on the command line must be "
        "under this directory. You can use "
        "this option to let 3c modify your project's own header files but not "
        "those of libraries outside your control."),
    cl::init(""), cl::cat(_3CCategory));

static cl::opt<bool> OptAllowSourcesOutsideBaseDir(
    "allow-sources-outside-base-dir",
    cl::desc("When a source file is outside the base directory, issue a "
             "warning instead of an error. This option is intended to be used "
             "temporarily until you fix your project setup and may be removed "
             "in the future."),
    cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptWarnRootCause(
    "warn-root-cause",
    cl::desc("Emit warnings indicating root causes of unchecked pointers."),
    cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool>
    OptWarnAllRootCause("warn-all-root-cause",
                        cl::desc("Emit warnings for all root causes, "
                                 "even those unlikely to be interesting."),
                        cl::init(false), cl::cat(_3CCategory));

// https://clang.llvm.org/doxygen/classclang_1_1VerifyDiagnosticConsumer.html#details
//
// Analogous to the -verify option of `clang -cc1`, but currently applies only
// to the rewriting phase (because it is the only phase that generates
// diagnostics, except for the declaration merging diagnostics that are
// currently fatal). No checking of diagnostics from the other phases is
// performed. We cannot simply have the caller pass `-extra-arg=-Xclang
// -extra-arg=-verify` because that would expect each phase to produce the same
// set of diagnostics.
static cl::opt<bool> OptVerifyDiagnosticOutput(
    "verify",
    cl::desc("Verify diagnostic output (for automated testing of 3c)."),
    cl::init(false), cl::cat(_3CCategory), cl::Hidden);

// In the future, we may enhance this to write the output to individual files.
// For now, the user has to copy and paste the correct portions of stderr.
static cl::opt<bool> OptDumpUnwritableChanges(
    "dump-unwritable-changes",
    cl::desc("When 3c generates changes to a file it cannot write (due to "
             "stdout mode or implementation limitations), dump the new version "
             "of the file to stderr for troubleshooting."),
    cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptAllowUnwritableChanges(
    "allow-unwritable-changes",
    // "3C" for the software in general, "3c" for this frontend. :/
    cl::desc("When 3c generates changes to a file it cannot write (due to "
             "stdout mode or implementation limitations), issue a warning "
             "instead of an error. This option is intended to be used "
             "temporarily until you fix the root cause of the problem (by "
             "correcting your usage of stdout mode or reporting the "
             "implementation limitation to the 3C team to get it fixed) and "
             "may be removed in the future."),
    cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptAllowRewriteFailures(
    "allow-rewrite-failures",
    cl::desc("When 3c fails to make a rewrite to a source file (typically "
             "because of macros), issue a warning instead of an error. This "
             "option is intended to be used temporarily until you change your "
             "code to allow 3c to work or you report the problem to the 3C "
             "team to get it fixed; the option may be removed in the future. "
             "Note that some kinds of rewrite failures currently generate "
             "warnings regardless of this option, due to known bugs that "
             "affect common use cases."),
    cl::init(false), cl::cat(_3CCategory));

#ifdef FIVE_C
static cl::opt<bool> OptRemoveItypes(
    "remove-itypes",
    cl::desc("Remove unneeded interoperation type annotations."),
    cl::init(false), cl::cat(_3CCategory));

static cl::opt<bool> OptForceItypes(
    "force-itypes",
    cl::desc("Use interoperation types instead of regular checked pointers. "),
    cl::init(false), cl::cat(_3CCategory));
#endif

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);

  // Initialize targets for clang module support.
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  CommonOptionsParser OptionsParser(argc, (const char **)(argv), _3CCategory,
                                    HelpOverview);
  // Setup options.
  struct _3COptions CcOptions;
  CcOptions.BaseDir = OptBaseDir.getValue();
  CcOptions.AllowSourcesOutsideBaseDir = OptAllowSourcesOutsideBaseDir;
  CcOptions.EnablePropThruIType = OptEnablePropThruIType;
  CcOptions.HandleVARARGS = OptHandleVARARGS;
  CcOptions.DumpStats = OptDumpStats;
  CcOptions.OutputPostfix = OptOutputPostfix.getValue();
  CcOptions.OutputDir = OptOutputDir.getValue();
  CcOptions.Verbose = OptVerbose;
  CcOptions.DumpIntermediate = OptDumpIntermediate;
  CcOptions.ConstraintOutputJson = OptConstraintOutputJson.getValue();
  CcOptions.StatsOutputJson = OptStatsOutputJson.getValue();
  CcOptions.WildPtrInfoJson = OptWildPtrInfoJson.getValue();
  CcOptions.PerPtrInfoJson = OptPerPtrWILDInfoJson.getValue();
  CcOptions.AddCheckedRegions = OptAddCheckedRegions;
  CcOptions.EnableAllTypes = OptAllTypes;
  CcOptions.DisableCCTypeChecker = OptDiableCCTypeChecker;
  CcOptions.WarnRootCause = OptWarnRootCause;
  CcOptions.WarnAllRootCause = OptWarnAllRootCause;
  CcOptions.VerifyDiagnosticOutput = OptVerifyDiagnosticOutput;
  CcOptions.DumpUnwritableChanges = OptDumpUnwritableChanges;
  CcOptions.AllowUnwritableChanges = OptAllowUnwritableChanges;
  CcOptions.AllowRewriteFailures = OptAllowRewriteFailures;

#ifdef FIVE_C
  CcOptions.RemoveItypes = OptRemoveItypes;
  CcOptions.ForceItypes = OptForceItypes;
#endif

  //Add user specified function allocators
  std::string Malloc = OptMalloc.getValue();
  if (!Malloc.empty()) {
    std::string Delimiter = ",";
    size_t Pos = 0;
    std::string Token;
    while ((Pos = Malloc.find(Delimiter)) != std::string::npos) {
      Token = Malloc.substr(0, Pos);
      CcOptions.AllocatorFunctions.push_back(Token);
      Malloc.erase(0, Pos + Delimiter.length());
    }
    Token = Malloc;
    CcOptions.AllocatorFunctions.push_back(Token);
  } else
    CcOptions.AllocatorFunctions = {};

  // Create 3C Interface.
  //
  // See clang/docs/checkedc/3C/clang-tidy.md#_3c-name-prefix
  // NOLINTNEXTLINE(readability-identifier-naming)
  std::unique_ptr<_3CInterface> _3CInterfacePtr(
      _3CInterface::create(CcOptions, OptionsParser.getSourcePathList(),
                           &(OptionsParser.getCompilations())));
  if (!_3CInterfacePtr) {
    // _3CInterface::create has already printed an error message. Just exit.
    return 1;
  }
  // See clang/docs/checkedc/3C/clang-tidy.md#_3c-name-prefix
  // NOLINTNEXTLINE(readability-identifier-naming)
  _3CInterface &_3CInterface = *_3CInterfacePtr;

  if (OptVerbose)
    errs() << "Calling Library to building Constraints.\n";
  // First build constraints.
  if (!_3CInterface.buildInitialConstraints()) {
    errs() << "Failure occurred while trying to build constraints. Exiting.\n";
    return 1;
  }

  if (OptVerbose) {
    errs() << "Finished Building Constraints.\n";
    errs() << "Trying to solve Constraints.\n";
  }

  // Next solve the constraints.
  if (!_3CInterface.solveConstraints()) {
    errs() << "Failure occurred while trying to solve constraints. Exiting.\n";
    return 1;
  }

  if (OptVerbose) {
    errs() << "Finished solving constraints.\n";
    errs() << "Trying to rewrite the converted files back.\n";
  }

  // Write all the converted files back.
  if (!_3CInterface.writeAllConvertedFilesToDisk()) {
    errs() << "Failure occurred while trying to rewrite converted files back. "
              "Exiting.\n";
    return 1;
  }

  return 0;
}
