// Gestion d'unité - JSON file helpers. Server only; files live under $profile:.

//! Indented JSON so the files stay easy to read by hand or by an AI.
class CGQC_GU_PrettyJsonSaveContext : JsonSaveContext
{
	protected ref PrettyJsonSaveContainer m_Pretty;

	void CGQC_GU_PrettyJsonSaveContext(bool skipEmptyObjects = true)
	{
		m_Pretty = new PrettyJsonSaveContainer();
		m_Pretty.SetIndent(" ", 2);
		SetContainer(m_Pretty);
	}
}

//! JSON to a string, for RPC payloads (server -> client).
class CGQC_GU_JsonStringSaveContext : JsonSaveContext
{
	string Export()
	{
		return SaveToString();
	}
}

class CGQC_GU_Json
{
	//------------------------------------------------------------------------------------------------
	//! Safe write: save to <path>.tmp, check it parses, keep the previous file as <path>.bak,
	//! then put the new file in place. Loading falls back to .tmp then .bak if <path> is unreadable.
	static bool SafeSave(JsonSaveContext ctx, string path)
	{
		string tmp = path + ".tmp";
		string bak = path + ".bak";

		if (FileIO.FileExists(tmp))
			FileIO.DeleteFile(tmp);

		if (!ctx.SaveToFile(tmp))
		{
			CGQC_GU_Log.Error("Écriture impossible : " + tmp);
			return false;
		}

		JsonLoadContext check = new JsonLoadContext();
		if (!check.LoadFromFile(tmp))
		{
			CGQC_GU_Log.Error("Fichier temporaire illisible, ancien fichier conservé : " + tmp);
			FileIO.DeleteFile(tmp);
			return false;
		}

		if (FileIO.FileExists(path))
		{
			if (FileIO.FileExists(bak))
				FileIO.DeleteFile(bak);

			FileIO.CopyFile(path, bak);
			FileIO.DeleteFile(path);
		}

		if (!FileIO.CopyFile(tmp, path))
		{
			CGQC_GU_Log.Error("Copie impossible vers " + path + " ; la version neuve reste dans " + tmp);
			return false;
		}

		FileIO.DeleteFile(tmp);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Open <path>, or its .tmp / .bak copy when the main file is missing or broken.
	static JsonLoadContext Open(string path)
	{
		array<string> candidates = { path, path + ".tmp", path + ".bak" };
		foreach (int i, string candidate : candidates)
		{
			if (!FileIO.FileExists(candidate))
				continue;

			JsonLoadContext ctx = new JsonLoadContext();
			if (ctx.LoadFromFile(candidate))
			{
				if (i > 0)
					CGQC_GU_Log.Warn("Fichier principal illisible, chargé depuis " + candidate);

				return ctx;
			}

			CGQC_GU_Log.Error("JSON illisible : " + candidate);
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Directory part of a path ending in a file name.
	static void EnsureDirectory(string dir)
	{
		if (!FileIO.FileExists(dir))
			FileIO.MakeDirectory(dir);
	}
}
