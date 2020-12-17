package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.security.CertificateStatus;
	
	/**
	 * ...
	 * @author Huw Pritchard
	 */
	public class Main extends Sprite 
	{
		
		public function Main() 
		{
			if (CertificateStatus.EXPIRED == "expired")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.INVALID == "invalid")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.INVALID_CHAIN == "invalidChain")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.NOT_YET_VALID == "notYetValid")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.PRINCIPAL_MISMATCH == "principalMismatch")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.REVOKED == "revoked")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.TRUSTED == "trusted")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.UNKNOWN == "unknown")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
			
			if (CertificateStatus.UNTRUSTED_SIGNERS == "untrustedSigners")
			{
				trace("Passed");
			}
			else
			{
				trace("Failed");
			}
		}
		
	}
	
}