// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class libssh : ModuleRules
{
    public libssh(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        PublicDefinitions.Add("WITH_LIBSSH=1");

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

        string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", ".."));
        string ThirdPartyPath = System.IO.Path.Combine(ModuleDirectory, Target.Platform.ToString());

        if (Target.bBuildEditor)
        {
            // Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Win64", "lib", "ssh.lib"));

            // Load and copy dlls
            string sshDLLFile = System.IO.Path.Combine(ModuleDirectory, Target.Platform.ToString(), "bin", "ssh.dll");
            string pthreadsDLLFile = System.IO.Path.Combine(ModuleDirectory, Target.Platform.ToString(), "bin", "pthreadVC3.dll");
            if (File.Exists(sshDLLFile) && File.Exists(pthreadsDLLFile))
            {
                PublicDelayLoadDLLs.Add("ssh.dll");
                PublicDelayLoadDLLs.Add("pthreadVC3.dll");
                RuntimeDependencies.Add(sshDLLFile);
                RuntimeDependencies.Add(pthreadsDLLFile);

                CopyToBinaries(sshDLLFile, BaseDirectory, Target);
                CopyToBinaries(pthreadsDLLFile, BaseDirectory, Target);
            }
            else
            {
                throw new BuildException("ssh.dll and/or pthreadVC3.dll not found. Expected locations: " + sshDLLFile + ", " + pthreadsDLLFile);
            }
        }

        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
        {
            // Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, Target.Platform.ToString(), "lib", "ssh.lib"));

            // Load and copy dlls
            string sshDLLFile = System.IO.Path.Combine(ModuleDirectory, Target.Platform.ToString(), "bin", "ssh.dll");
            string pthreadsDLLFile = System.IO.Path.Combine(ModuleDirectory, Target.Platform.ToString(), "bin", "pthreadVC3.dll");
            if (File.Exists(sshDLLFile) && File.Exists(pthreadsDLLFile))
            {
                PublicDelayLoadDLLs.Add("ssh.dll");
                PublicDelayLoadDLLs.Add("pthreadVC3.dll");
                // Only necessary of dlls need to be packaged with a game.
                RuntimeDependencies.Add(sshDLLFile);
                RuntimeDependencies.Add(pthreadsDLLFile);

                CopyToBinaries(sshDLLFile, BaseDirectory, Target);
                CopyToBinaries(pthreadsDLLFile, BaseDirectory, Target);
            }
            else
            {
                throw new BuildException("ssh.dll and/or pthreadVC3.dll not found. Expected locations: " + sshDLLFile + ", " + pthreadsDLLFile);
            }


        }
    }

    /// <summary>
    /// copies the given dll file to the plugins binaries folder
    /// </summary>
    /// <param name="Filepath">path to the dll file</param>
    /// <param name="Target">Target to get Platform from</param>
    private void CopyToBinaries(string Filepath, string PluginBaseDir, ReadOnlyTargetRules Target)
    {
        string BinariesDirectory = System.IO.Path.Combine(PluginBaseDir, "Binaries", Target.Platform.ToString());
        string Filename = Path.GetFileName(Filepath);

        if (!Directory.Exists(BinariesDirectory))
        {
            Directory.CreateDirectory(BinariesDirectory);
        }

        if (File.Exists(Path.Combine(BinariesDirectory, Filename)) == false)
        {
            File.Copy(Filepath, System.IO.Path.Combine(BinariesDirectory, Filename), true);
        }
    }
}
