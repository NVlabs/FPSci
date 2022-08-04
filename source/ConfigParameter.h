#pragma once
#include <G3D/G3D.h>

class FpsciParameter : public ReferenceCountedObject {
public:
	FpsciParameter();
protected:
	virtual void validate(T v) {};
};

template <typename T>
class ConfigParameter : public FpsciParameter {
public:
	const String name;				///< Name this parameter will be referred to by
	T value;						///< The value of this parameter
	const T defValue;				///< This is storage for the default value
	const bool required = false;	///< Is this a required parameter?

	ConfigParameter(String name, T v, bool req=false) : name(name), value(v), defValue(v), required(req) {}

	// Get the value from Any
	void fromAny(AnyTableReader reader, String msg=format("Value %s must be specified!", name)) {
		if (required) reader.get(name, value, msg);
		else reader.getIfPresent(name, value);
		validate(value);
	}

	// Write the value to Any
	void addToAny(Any a, bool forceAll = false) const {
		if (forceAll || defValue != value) a[name] = value;
	}
};

template <typename T>
class NumericConfigParameter : public ConfigParameter<T> {
public:
	NumericConfigParameter(String name, T v, T min = -(1e12), T max = 1e12, bool req = false) : ConfigParameter(name, v, req), min(min), max(max) {};
protected:
	T min;
	T max;
	void validate(T v) {
		if (v < min) throw format("Parameter %s cannot be < %f!", ConfigParameter::name, min).c_str();
		if (v > max) throw format("Parameter %s cannot be > %f!", ConfigParameter::name, max).c_str();
	}
};

template <typename T>
class OptionsConfigParameter : public ConfigParameter<T> {
public:
	OptionsConfigParameter(String name, T v, Array<T> options, bool req = false) : ConfigParameter(name, v, req), options(options) {};
protected:
	Array<T> options;
	void validate(T v) {
		if (!options.contains(v)) {
			String errString = format("\"%s\" value \"%s\" is invalid, must be specified as one of the valid options (", ConfigParameter::name, v);
			for (T o : options) errString += format("\"%s\", ", o);
			errString = errString.substr(0, errString.length() - 2) + ")!";
			throw errString.c_str();
		}
	}
};