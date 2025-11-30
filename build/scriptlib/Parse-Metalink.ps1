param([String]$filename="")

[xml]$xmlData = Get-Content -Path $filename
foreach ($node in $xmlData.ChildNodes) {
	if ($node.LocalName -eq "metalink") {
		foreach ($file in $node.ChildNodes) {
			$fields = @()
			$fields += $file.name
			$fields += $file.size
			$fields += $file.hash.InnerText
			foreach ($url in $file.ChildNodes) {
				if ($url.LocalName -eq "url") {
					$fields += '"' + $url.InnerText + '"'
				}
			}
			Write-Output ($fields -join " ")
		}
	}
}
