// Gestion d'unité - request payload: a flat set of string values, sent as JSON {"p": {"key": "value", ...}}.
// Lists are comma-separated strings ("commandement,zeus"). Every value is re-validated on the server.

class CGQC_GU_Payload
{
	protected ref map<string, string> m_mValues = new map<string, string>();

	//------------------------------------------------------------------------------------------------
	CGQC_GU_Payload Set(string key, string value)
	{
		m_mValues.Set(key, value);
		return this;
	}

	//------------------------------------------------------------------------------------------------
	string Get(string key)
	{
		string value;
		m_mValues.Find(key, value);
		return value;
	}

	//------------------------------------------------------------------------------------------------
	bool Has(string key)
	{
		return m_mValues.Contains(key);
	}

	//------------------------------------------------------------------------------------------------
	bool GetBool(string key)
	{
		string v = CGQC_GU_Str.Lower(Get(key));
		return v == "1" || v == "true" || v == "oui";
	}

	//------------------------------------------------------------------------------------------------
	void GetList(string key, notnull array<string> outList)
	{
		array<string> parts = new array<string>();
		Get(key).Split(",", parts, true);
		foreach (string p : parts)
		{
			string t = p.Trim();
			if (!t.IsEmpty() && !outList.Contains(t))
				outList.Insert(t);
		}
	}

	//------------------------------------------------------------------------------------------------
	string ToJson()
	{
		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("p", m_mValues);
		return ctx.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Payload FromJson(string json)
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		if (json.IsEmpty())
			return p;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
			return p;

		map<string, string> values = new map<string, string>();
		if (ctx.ReadValue("p", values) && values)
			p.m_mValues = values;

		return p;
	}
}
