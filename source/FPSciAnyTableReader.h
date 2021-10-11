#pragma once
#include <G3D/G3D.h>


class FPSciAnyTableReader : public AnyTableReader {
public:
	// Pass through to regular constructor
	FPSciAnyTableReader(const Any& any) : AnyTableReader(any) {};

	// The methods below allow serializing G3D types like Point2, Vector2, Point3, Vector3, Color3, ... from arrays
	// This allows JSON-serialized Any to be read back into these types successfully

	void get(const String& s, Vector2& v, const String& errMsg = "")  {
		try {
			AnyTableReader::get(s, v, errMsg);
		}
		catch (ParseError e) {
			Array<float> arr;
			try {
				AnyTableReader::get(s, arr);		// We failed to get the Vector2, try to get an array (of length 2) instead
				if (arr.length() != 2) {
					e.message = format("\nArray specified for \"%s\" (Vector2) is of length %d (must be of length 2)", s.c_str(), arr.length());
					throw e;
				}
				v.x = arr[0]; v.y = arr[1];
			}
			catch (ParseError e1) {
				throw e;
			}
		}
	}

	void get(const String& s, Vector3& v, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, v, errMsg);
		}
		catch (ParseError e) {
			Array<float> arr;
			try {
				AnyTableReader::get(s, arr);		// We failed to get the Vector2, try to get an array (of length 3) instead
				if (arr.length() != 3) {
					e.message = format("\nArray specified for \"%s\" (Vector3) is of length %d (must be of length 3)", s.c_str(), arr.length());
					throw e;
				}
				v.x = arr[0]; v.y = arr[1]; v.z = arr[2];
			}
			catch (ParseError e1) {
				throw e;
			}
		}
	}

	void get(const String& s, Color3& c, const String& errMsg = "") {
		Vector3 v;
		get(s, v, errMsg);
		c = Color3(v);
	}

	void get(const String& s, Vector4& v, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, v, errMsg);
		}
		catch (ParseError e) {
			Array<float> arr;
			try {
				AnyTableReader::get(s, arr);		// We failed to get the Vector2, try to get an array (of length 4) instead
				if (arr.length() != 4) {
					e.message = format("\nArray specified for \"%s\" (Vector4) is of length %d (must be of length 4)", s.c_str(), arr.length());
					throw e;
				}
				v.x = arr[0]; v.y = arr[1]; v.z = arr[2]; v.w = arr[3];
			}
			catch (ParseError e1) {
				throw e;
			}
		}
	}

	void get(const String& s, Color4& c, const String& errMsg = "") {
		Vector4 v4;
		try {
			get(s, v4, errMsg);
			c = Color4(v4);
		}
		catch (ParseError e) {
			Vector3 v3;
			try {
				get(s, v3, errMsg);
				c.r = v3.x; c.g = v3.y; c.b = v3.z; c.a = 1.f;
			}
			catch (ParseError e1) {
				e.message = format("\nFailed to parse \"%s\" (Color4) as Color or array of length 3 or 4!", s.c_str());
				throw e;
			}
		}
	}

	// Explicitly support arrays for the types above (this doesn't work well with a tempalted solution)

	void get(const String& s, Array<Vector2>& v, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, v, errMsg);	// Try normal read approach
		}
		catch (ParseError e) {
			// Attempt to read Vector2, Vector3, Color3, Vector4, Color4 from array
			const Any anyArray = any()[s];
			for (int i = 0; i < anyArray.size(); i++) {
				// Try to get as array
				Array<float> a;
				try {
					anyArray[i].getArray(a);
					v[i] = Vector2(a[0], a[1]);
				}
				catch (ParseError e) {
					e.message = format("Failed to parse \"%s\" as Array<Vector2> from Any!", s.c_str());
					throw e;
				}
			}
		}
	}

	void get(const String& s, Array<Vector3>& v, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, v, errMsg);	// Try normal read approach
		}
		catch (ParseError e) {
			// Attempt to read Vector2, Vector3, Color3, Vector4, Color4 from array
			const Any anyArray = any()[s];
			for (int i = 0; i < anyArray.size(); i++) {
				// Try to get as array
				Array<float> a;
				try {
					anyArray[i].getArray(a);
					v[i] = Vector3(a[0], a[1], a[2]);
				}
				catch (ParseError e) {
					e.message = format("Failed to parse \"%s\" as Array<Vector3> from Any!", s.c_str());
					throw e;
				}
			}
		}
	}

	void get(const String& s, Array<Vector4>& v, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, v, errMsg);	// Try normal read approach
		}
		catch (ParseError e) {
			// Attempt to read Vector2, Vector3, Color3, Vector4, Color4 from array
			const Any anyArray = any()[s];
			for (int i = 0; i < anyArray.size(); i++) {
				// Try to get as array
				Array<float> a;
				try {
					anyArray[i].getArray(a);
					v[i] = Vector4(a[0], a[1], a[2], a[3]);
				}
				catch (ParseError e) {
					e.message = format("Failed to parse \"%s\" as Array<Vector4> from Any!", s.c_str());
					throw e;
				}
			}
		}
	}

	void get(const String& s, Array<Color3>& arr, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, arr, errMsg);	// Try normal read approach
		}
		catch (ParseError e) {
			// Attempt to read Vector2, Vector3, Color3, Vector4, Color4 from array
			const Any anyArray = any()[s];
			for (int i = 0; i < anyArray.size(); i++) {
				// Try to get as array
				Array<float> v;
				try {
					anyArray[i].getArray(v);
					arr[i] = Color3(v[0], v[1], v[2]);
				}
				catch (ParseError e) {
					e.message = format("Failed to parse \"%s\" as Array<Color3> from Any!", s.c_str()); 
					throw e;
				}
			}
		}
	}

	void get(const String& s, Array<Color4>& arr, const String& errMsg = "") {
		try {
			AnyTableReader::get(s, arr, errMsg);	// Try normal read approach
		}
		catch (ParseError e) {
			// Attempt to read Vector2, Vector3, Color3, Vector4, Color4 from array
			const Any anyArray = any()[s];
			for (int i = 0; i < anyArray.size(); i++) {
				// Try to get as array
				Array<float> v;
				try {
					anyArray[i].getArray(v);
					arr[i] = Color4(v[0], v[1], v[2], v[3]);
				}
				catch (ParseError e) {
					e.message = format("Failed to parse \"%s\" as Array<Color4> from Any!", s.c_str());
					throw e;
				}
			}
		}
	}

	// Capture generic get(IfPresent) calls here to make sure we call our methods

	// Method required to pass through cases unhandled above
	template<class ValueType>
	void get(const String& s, ValueType& v, const String& errMsg = "") {
		AnyTableReader::get(s, v, errMsg);
	}

	// Method required to redirect getIfPresent() to the FPSciAnyTableReader
	template<class ValueType>
	bool getIfPresent(const String& s, ValueType& v) {
		if (any().containsKey(s)) {
			get(s, v);
			return true;
		}
		else {
			return false;
		}
	}

	// Utility Methods

	// Currently unused, allows getting scalar or vector from Any
	template <class T>
	static void getArrayFromElement(const String& name, Array<T>& output) {
		bool valid = true;
		T value;
		try {
			Array<T> arr;
			get(name, arr);					// Try to read an array directly
			if (arr.size() < output.size()) {		// Check for under size (critical)
				throw format("\"%s\" array of length %d is underspecified (should be of length %d)!", name.c_str(), arr.size(), output.size());
			}
			else if (arr.size() != output.size()) {	// Check for over size (warn)
				logPrintf("WARNING: array specified for \"%s\" is of length %d, but should be %d (using first %d elements)...\n", name.c_str(), arr.size(), output.size(), output.size());
			}
			// Copy over array elements
			for (int i = 0; i < output.size(); i++) {
				output[i] = arr[i];
			}
		}
		catch (ParseError& e) {
			// Failed to read array, try to duplicate a single value
			valid = getIfPresent(name, value);
			if (valid) {
				for (int i = 0; i < output.size(); i++) {
					output[i] = value;
				}
			}
		}
	}
};