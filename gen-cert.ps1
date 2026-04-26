$pwd = ConvertTo-SecureString -String 'DeskDisplay2026!' -Force -AsPlainText
$cert = New-SelfSignedCertificate -Type CodeSigningCert `
    -Subject 'CN=DeskDisplay' `
    -CertStoreLocation 'Cert:\CurrentUser\My' `
    -KeyExportPolicy Exportable `
    -KeySpec Signature `
    -NotAfter (Get-Date).AddYears(5)
Export-PfxCertificate -Cert $cert -FilePath 'D:\PycharmProjects\DeskDisplay\DeskDisplay.pfx' -Password $pwd
Write-Output "SUCCESS"
Write-Output "Thumbprint: $($cert.Thumbprint)"
