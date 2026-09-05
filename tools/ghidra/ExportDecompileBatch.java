// Export many functions' initial Ghidra C output in ONE headless run.
//
// Neden var: her analyzeHeadless cagrisi JVM + proje acilisi yuzunden
// yaklasik bir dakika harciyor.  Tek tek cagirmak 30 fonksiyon icin yarim
// saat demek; bu betik ayni acilista hepsini cikariyor.
//
// Kullanim (tools/ghidra_headless.sh uzerinden):
//   -postScript ExportDecompileBatch.java <adres_listesi> <cikti_dizini>
// Adres listesi satir basina "0xADRES ad" bicimindedir; ad verilmezse
// Ghidra'nin kendi adi kullanilir.
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
                        Function function = getFunctionAt(address);
                        if (function == null) {
                            println("ATLANDI (fonksiyon yok): " + addrText);
                            fail++;
                            continue;
                        }
                        String name = parts.length > 1 ? parts[1] : function.getName();

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
                        println("HATA " + addrText + ": " + e.getMessage());
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
