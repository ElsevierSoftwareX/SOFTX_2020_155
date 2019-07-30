//
// Created by jonathan.hanks on 10/31/17.
//

#ifndef DAQD_FE_STREAM_GENERATOR_HH
#define DAQD_FE_STREAM_GENERATOR_HH

#include <tr1/memory>

#include <iostream>
#include <sstream>
#include <string>

#include <stdexcept>

#include "str_split.hh"

class SimChannel
{
private:
    std::string name_;
    int chtype_;
    int rate_;
    int chnum_;
    int dcuid_;
public:
    SimChannel(): name_(""), chtype_(0), rate_(0), chnum_(0), dcuid_(0) {}
    SimChannel(std::string name, int chtype, int rate, int chnum, int dcuid=0):
            name_(name), chtype_(chtype), rate_(rate), chnum_(chnum), dcuid_(dcuid) {}
    SimChannel(const SimChannel& other):
            name_(other.name_), chtype_(other.chtype_), rate_(other.rate_),
            chnum_(other.chnum_), dcuid_(other.dcuid_) {}

    SimChannel& operator=(const SimChannel& other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            chtype_ = other.chtype_;
            rate_ = other.rate_;
            chnum_ = other.chnum_;
            dcuid_ = other.dcuid_;
        }
        return *this;
    }

    int data_type() const { return chtype_; }
    int data_rate() const { return rate_; }
    int channel_num() const { return chnum_; }
    int dcuid() const { return dcuid_; }

    const std::string& name() const { return name_; }
};

class Generator
{
protected:
    virtual int data_type() const = 0;
    virtual int data_rate() const = 0;
    virtual const std::string& channel_base_name() const = 0;

    virtual std::string other_params() const { return ""; }
public:
    virtual std::string generator_name() const = 0;
    virtual std::string full_channel_name() const
    {
        std::ostringstream os;
        os << channel_base_name() << "--" << generator_name() << other_params() << "--" << data_type() << "--" << data_rate();
        return os.str();
    }

    virtual size_t bytes_per_sec() const = 0;

    virtual char* generate(int gps_sec, int gps_nano, char *out) = 0;

    virtual void output_ini_entry(std::ostream& os) = 0;
    virtual void output_par_entry(std::ostream& os) = 0;
};

typedef std::tr1::shared_ptr<Generator> GeneratorPtr;

namespace Generators {
    class SimChannelGenerator : public Generator {
    protected:
        SimChannel ch_;

        virtual int data_type() const { return ch_.data_type(); };
        virtual int data_rate() const { return ch_.data_rate(); };
        virtual const std::string& channel_base_name() const { return ch_.name(); };
    public:
        SimChannelGenerator(const SimChannel& ch):
                ch_(ch) {}

        virtual size_t bytes_per_sec() const
        {
            if (ch_.data_type() != 2 && ch_.data_type() != 4)
                throw std::runtime_error("Unsupported type");
            return ch_.data_rate()*4;
        }

        virtual void output_ini_entry(std::ostream& os)
        {
            os << "[" << full_channel_name() << "]\n";
            os << "datarate=" << data_rate() << "\n";
            os << "datatype=" << data_type() << "\n";
            os << "chnnum=" << ch_.channel_num() << "\n";
        }

        virtual void output_par_entry(std::ostream& os)
        {
            if (data_rate() != 2048)
                throw std::runtime_error("Cannot generate a par entry for a non 2k channel right now");
            if (data_type() != 4)
                throw std::runtime_error("Cannot generate a par entry for a non float channel");
            os << "[" << full_channel_name() << "]\n";
            os << "ifoid = 1\n";
            os << "rmid = " << ch_.dcuid() << "\n";
            os << "dcuid = " << 16 << "\n";
            os << "chnnum = " << ch_.channel_num() << "\n";
            os << "datatype = " << data_type() << "\n";
            os << "datarate = " << data_rate() << "\n";
        }
    };

    
    
    class GPSSecondGenerator: public SimChannelGenerator
    {
    public:
        GPSSecondGenerator(const SimChannel& ch):
                SimChannelGenerator(ch) {}

        std::string generator_name() const { return "gpssec"; }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            int *out_ = reinterpret_cast<int*>(out);
            for (int i = 0; i < rate; ++i)
            {
                *out_ = gps_sec;
                ++out_;
            }
            return reinterpret_cast<char*>(out_);
        }
    };

    template <typename T>
    class GPSSecondWithOffset: public SimChannelGenerator
    {
        int offset_;
    public:
        GPSSecondWithOffset(const SimChannel& ch, int offset):
                SimChannelGenerator(ch), offset_(offset) {}

        std::string generator_name() const { return "gpssoff1p"; }

        std::string other_params() const
        {
            std::ostringstream os;
            os << "--" << offset_;
            return os.str();
        }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            T *out_ = reinterpret_cast<T*>(out);
            for (int i = 0; i < rate; ++i)
            {
                *out_ = static_cast<T>(gps_sec + offset_);
                ++out_;
            }
            return reinterpret_cast<char*>(out_);
        }
    };

    template <typename T>
    class GPSMod100kSecWithOffset: public SimChannelGenerator
    {
        int offset_;
    public:
        GPSMod100kSecWithOffset(const SimChannel& ch, int offset):
                SimChannelGenerator(ch), offset_(offset) {}

        std::string generator_name() const { return "gpssmd100koff1p"; }

        std::string other_params() const
        {
            std::ostringstream os;
            os << "--" << offset_;
            return os.str();
        }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            T *out_ = reinterpret_cast<T*>(out);
            for (int i = 0; i < rate; ++i)
            {
                *out_ = static_cast<T>((gps_sec%100000) + offset_);
                ++out_;
            }
            return reinterpret_cast<char*>(out_);
        }
    };

    template <typename T>
    class StaticValue: public SimChannelGenerator
    {
        T value_;
    public:
        StaticValue(const SimChannel& ch, T value):
                SimChannelGenerator(ch), value_(value) {}

        std::string generator_name() const { return "static"; }

        std::string other_params() const
        {
            std::ostringstream os;
            os << "--" << value_;
            return os.str();
        }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            T *out_ = reinterpret_cast<T*>(out);
            std::fill(out_, out_ + rate, value_);
            return reinterpret_cast<char*>(out_ + rate);
        }
    };
}

GeneratorPtr create_generator(const std::string& generator, const SimChannel& ch)
{
    if (ch.data_type() != 2)
        throw std::runtime_error("Invalid/unsupported data type for a generator");
    if (generator == "gps_sec") {
        return GeneratorPtr(new Generators::GPSSecondGenerator(ch));
    }
    throw std::runtime_error("Unknown generator type");
}

GeneratorPtr create_generator(const std::string& channel_name)
{
    std::vector<std::string> parts = split(channel_name, "--");
    if (parts.size() < 4)
        throw std::runtime_error("Generator name has too few parts, invalid input");
    int data_type = 0;
    {
        std::istringstream is (parts[parts.size()-2]);
        is >> data_type;
    }
    int rate = 0;
    {
        std::istringstream is (parts[parts.size()-1]);
        is >> rate;
    }
    if (!(data_type == 2 || data_type == 4) || rate < 16)
        throw std::runtime_error("Invalid data type or rate found");
    std::string& base = parts[0];
    std::string& name = parts[1];
    int arg_count = parts.size()-4; // ignore base channel name, data type, rate
    if (name == "gpssoff1p" && arg_count == 1)
    {
        std::istringstream is(parts[2]);
        int offset = 0;
        is >> offset;
        if (data_type == 2)
            return GeneratorPtr(new Generators::GPSSecondWithOffset<int>(SimChannel(base, data_type, rate, 0), offset));
        else
            return GeneratorPtr(new Generators::GPSSecondWithOffset<float>(SimChannel(base, data_type, rate, 0), offset));

    }
    else if (name == "gpssmd100koff1p" && arg_count == 1)
    {
        std::istringstream is(parts[2]);
        int offset = 0;
        is >> offset;
        if (data_type == 2)
            return GeneratorPtr(new Generators::GPSMod100kSecWithOffset<int>(SimChannel(base, data_type, rate, 0), offset));
        else
            return GeneratorPtr(new Generators::GPSMod100kSecWithOffset<float>(SimChannel(base, data_type, rate, 0), offset));
    }
    else
    {
        throw std::runtime_error("Unknown generator type");
    }
}

#endif //DAQD_FE_STREAM_GENERATOR_HH
