class CGQC_Log
{
	static void Dbg( string msg)
	{
		#ifdef CGQC_DEBUG
		Print(string.Format("[%1]", msg), LogLevel.NORMAL);
		#endif
	}
}