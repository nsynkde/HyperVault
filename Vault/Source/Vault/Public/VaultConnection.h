// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <libssh/libsshpp.hpp>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include "Windows/HideWindowsPlatformTypes.h"

/**
 * 
 */
class VAULT_API VaultConnection
{
public:

	static VaultConnection& Get();

	void Initialize();

	void Shutdown();

	bool ConnectToSftpServer(FString IpAddress, FString username, FString password);

	ssh_session ssh;
	sftp_session sftp;
	bool IsConnected;

private:
	int verify_knownhost(ssh_session session);
};

