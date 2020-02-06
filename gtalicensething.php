<?php

	$servername = "localhost";
	$username = "USERNAME";
	$password = "PASSWORD";
	$dbname = "DATABASE NAME";

	function GetTimeDifference($time, $time2, $date, $date2)
	{
		$Start = '' . $date . ' ' . $time . '.000';
		$End = '' . $date2 . ' ' . $time2 . '.000';
		$StartDate = new DateTime($Start);
		$EndDate = new DateTime($End);
		$Differnce = $StartDate->diff($EndDate);
		$minutes = $Differnce->days * 24 * 60;
		$minutes += $Differnce->h * 60;
		$minutes += $Differnce->i;
		return $minutes;
	}

	$mysqli = new mysqli($servername, $username, $password, $dbname);

	if ($mysqli->connect_error)
	{
		die("Connection failed: " . $mysqli->connect_error);
	}
	else
	{
		if (isset($_GET["key"]))
		{
			$Key = (string)$_GET["key"];
			if (preg_match("/([A-Za-z0-9]+)/", $Key))
			{
				if (preg_match('/<script>/', $Key))
				{
					echo "Keys are numbers and letters only.";
				}
				else if (preg_match('/%3Cscript%3E/', $Key))
				{
					echo "Keys are numbers and letters only.";
				}
				else
				{
					if (strlen($Key) < 32)
					{
						echo "Your key is too short.";
					}
					else if (strlen($Key) > 32)
					{
						echo "Your key is too long.";
					}
					else
					{
						$result = $mysqli->query("SELECT * FROM  `Keys`");
						while($row = $result->fetch_assoc())
						{
							if($Key == $row['Key'])
							{
								date_default_timezone_set('Europe/London');
								if($row['Activated'] == 1)
								{
									if(GetTimeDifference($row['Timestamp'], date("H:i:s"), $row['DATE'], date("Y-m-d")) > 1440) // 1440 is 24 hours (Meaning the key will reset every 24 hours)
									{
										$ID = intval($row['ID']);
										$IP = $_SERVER['REMOTE_ADDR'];
										$mysqli->query("UPDATE `$dbname`.`Keys` SET `Timestamp` = NOW(), `IP` = '$IP', `DATE` = NOW() WHERE `Keys`.`ID` = $ID;");
										$mysqli->close();
										die("Key is valid.");
									}
									else
									{
										$IP = (string)$_SERVER['REMOTE_ADDR'];
										if($IP == $row['IP'])
										{
											$ID = intval($row['ID']);
											$IP = $_SERVER['REMOTE_ADDR'];
											$mysqli->query("UPDATE `$dbname`.`Keys` SET `Timestamp` = NOW(), `IP` = '$IP', `DATE` = NOW() WHERE `Keys`.`ID` = $ID;");
											$mysqli->close();
											die("Key is valid.");
										}
										else
										{
											die("Key is being used.");
										}
									}
								}
							}
						}
						$mysqli->close();
						echo "Key is not valid.";
					}
				}
			}
			else
			{
				echo "Keys are numbers and letters only.";
			}
		}
		else
		{
			echo "Please enter your license key.";
		}
	}

?>