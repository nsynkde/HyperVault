// Fill out your copyright notice in the Description page of Project Settings.


#include "VaultConnection.h"

VaultConnection& VaultConnection::Get()
{
	static VaultConnection vaultConnection;
	return vaultConnection;
}

void VaultConnection::Initialize()
{
	//try
	//{
	//	if (IsConnected)
	//	{
	//		session.disconnect();
	//	}
	//	// TODO use settings mask or similiar for connection.
	//	session.setOption(SSH_OPTIONS_HOST, "192.168.1.228");
	//	session.setOption(SSH_OPTIONS_USER, "vault");
	//	session.connect();
	//	session.userauthPassword("TheVault!1");
	//	IsConnected = session.getErrorCode() == 0;
	//	if (IsConnected)
	//	{
	//		UE_LOG(LogVault, Log, TEXT("SSH connected successfully!"), session.getErrorCode());
	//	}
	//	else
	//	{
	//		UE_LOG(LogVault, Log, TEXT("SSH connection failed (%i): %s"), session.getErrorCode(), session.getError());
	//	}

	//	//channel = ssh::Channel(session);

	//}
	//catch (ssh::SshException e)
	//{
	//	FString error(e.getError().c_str());
	//	UE_LOG(LogVault, Error, TEXT("SSH failed! %s"), *error);
	//}

    ConnectToSftpServer(L"192.168.1.228", L"vault", L"TheVault!1");
}

void VaultConnection::Shutdown()
{
	IsConnected = false;
	ssh_disconnect(ssh);
	ssh_free(ssh);
}

bool VaultConnection::ConnectToSftpServer(FString IpAddress, FString username, FString password)
{
	int rc;

	ssh = ssh_new();
    if (ssh == NULL)
    {
        return false;
    }
	ssh_options_set(ssh, SSH_OPTIONS_HOST, *IpAddress);
	ssh_options_set(ssh, SSH_OPTIONS_USER, *username);

	rc = ssh_connect(ssh);
    if (rc != SSH_OK)
    {
        ssh_free(ssh);
        return false;
    }

    rc = ssh_userauth_password(ssh, TCHAR_TO_ANSI(*username), TCHAR_TO_ANSI(*password));
    if (rc != SSH_AUTH_SUCCESS)
    {
        UE_LOG(LogVault, Error, TEXT("Failed to authenticate using password. %s"), ssh_get_error(ssh));
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return false;
    }

    return true;

	sftp = sftp_new(ssh);
}
