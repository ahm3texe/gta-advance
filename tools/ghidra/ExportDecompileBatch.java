// Export many functions' initial Ghidra C output in ONE headless run.
//
// Why it exists: each analyzeHeadless invocation spends about a minute on the
// JVM plus opening the project. Invoking it one function at a time would mean
// half an hour for 30 functions; this script exports them all in one startup.
//
// Usage (through tools/ghidra_headless.sh):
//   -postScript ExportDecompileBatch.java <address_list> <output_directory>
// The address list has the form "0xADDRESS name [size]" per line; without a
// name, Ghidra's own name is used.
//
// If a size is given and there is no function at that address, THUMB MODE is
// set first: Ghidra's automatic analysis tries to decode some entries in ARM
// mode and gives up with "bad instruction data" (5 of 7 missed entries were
// like this). The TMode register must be set to 1, the region cleared and
// re-disassembled.
//
//@category GTAAdvance

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class ExportDecompileBatch extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected <address-list> <output-dir>");
        }

        File outDir = new File(args[1]);
        outDir.mkdirs();

        DecompInterface decompiler = new DecompInterface();
        int ok = 0;
        int fail = 0;
        try {
            decompiler.toggleCCode(true);
            decompiler.toggleSyntaxTree(true);
            decompiler.setSimplificationStyle("decompile");
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(decompiler.getLastMessage());
            }

            try (BufferedReader in = new BufferedReader(new FileReader(args[0]))) {
                String line;
                while ((line = in.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty() || line.startsWith("#")) {
                        continue;
                    }
                    String[] parts = line.split("\\s+");
                    String addrText = parts[0];
                    try {
                        Address address = toAddr(Long.decode(addrText));
                        String want = parts.length > 1 ? parts[1] : null;

                        // Boyut verilmisse THUMB ONARIMI kosulsuz yapilir.
                        // Ghidra bazi girisleri ARM kipinde cozup "bad
                        // instruction data" ile birakiyor; boyle bir
                        // because the function may be left over from a PREVIOUS run
                        // "yoksa olustur" yetmez, VARSA DA yeniden kurulur.
                        if (parts.length > 2) {
                            int span = Integer.decode(parts[2]);
                            Address end = address.add(span - 1);
                            ghidra.program.model.listing.ProgramContext ctx =
                                currentProgram.getProgramContext();
                            ghidra.program.model.lang.Register tmode =
                                ctx.getRegister("TMode");
                            if (tmode != null) {
                                removeFunctionAt(address);
                                clearListing(address, end);
                                ctx.setValue(tmode, address, end,
                                    java.math.BigInteger.ONE);
                                disassemble(address);
                                println("THUMB ONARILDI " + addrText);
                            }
                        }

                        Function function = getFunctionAt(address);
                        if (function == null) {
                            function = createFunction(address, want);
                            if (function == null) {
                                println("SKIPPED (could not create function): " + addrText);
                                fail++;
                                continue;
                            }
                            println("OLUSTURULDU " + addrText);
                        }
                        String name = want != null ? want : function.getName();

                        DecompileResults result =
                            decompiler.decompileFunction(function, 240, monitor);
                        if (!result.decompileCompleted()
                                || result.getDecompiledFunction() == null) {
                            println("BASARISIZ " + addrText + ": " + result.getErrorMessage());
                            fail++;
                            continue;
                        }

                        File output = new File(outDir, name + ".c");
                        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
                            writer.println("/* Initial Ghidra output; analysis aid,"
                                + " not accepted source code. */");
                            writer.println("/* Function: " + function.getName()
                                + " @ " + function.getEntryPoint() + " */");
                            writer.println(result.getDecompiledFunction().getC());
                        }
                        println("OK " + addrText + " -> " + output.getName());
                        ok++;
                    } catch (Exception e) {
                        println("ERROR " + addrText + ": " + e.getMessage());
                        fail++;
                    }
                }
            }
        } finally {
            decompiler.dispose();
        }
        println("TOPLU CIKARIM BITTI: " + ok + " basarili, " + fail + " basarisiz");
    }
}
