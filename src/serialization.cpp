#include "serialization.h"
#include "connection.h"
#include "discord-rpc.h"

// it's ever so slightly faster to not have to strlen the key
template <typename T>
void WriteKey(JsonWriter& w, T& k)
{
    w.Key(k, sizeof(T) - 1);
}

struct WriteObject {
    JsonWriter& writer;
    WriteObject(JsonWriter& w)
      : writer(w)
    {
        writer.StartObject();
    }
    template <typename T>
    WriteObject(JsonWriter& w, T& name)
      : writer(w)
    {
        WriteKey(writer, name);
        writer.StartObject();
    }
    ~WriteObject() { writer.EndObject(); }
};

struct WriteArray {
    JsonWriter& writer;
    template <typename T>
    WriteArray(JsonWriter& w, T& name)
      : writer(w)
    {
        WriteKey(writer, name);
        writer.StartArray();
    }
    ~WriteArray() { writer.EndArray(); }
};

template <typename T>
void WriteOptionalString(JsonWriter& w, T& k, const char* value)
{
    if (value) {
        w.Key(k, sizeof(T) - 1);
        w.String(value);
    }
}

void JsonWriteNonce(JsonWriter& writer, int nonce)
{
    WriteKey(writer, "nonce");
    char nonceBuffer[32]{};
    rapidjson::internal::i32toa(nonce, nonceBuffer);
    writer.String(nonceBuffer);
}

size_t JsonWriteRichPresenceObj(char* dest,
                                size_t maxLen,
                                int nonce,
                                int pid,
                                const DiscordRichPresence* presence)
{
    JsonWriter writer(dest, maxLen);

    {
        WriteObject top(writer);

        JsonWriteNonce(writer, nonce);

        WriteKey(writer, "cmd");
        writer.String("SET_ACTIVITY");

        {
            WriteObject args(writer, "args");

            WriteKey(writer, "pid");
            writer.Int(pid);

            {
                WriteObject activity(writer, "activity");

                WriteOptionalString(writer, "state", presence->state);
                WriteOptionalString(writer, "details", presence->details);

                if (presence->startTimestamp || presence->endTimestamp) {
                    WriteObject timestamps(writer, "timestamps");

                    if (presence->startTimestamp) {
                        WriteKey(writer, "start");
                        writer.Int64(presence->startTimestamp);
                    }

                    if (presence->endTimestamp) {
                        WriteKey(writer, "end");
                        writer.Int64(presence->endTimestamp);
                    }
                }

                if (presence->largeImageKey || presence->largeImageText ||
                    presence->smallImageKey || presence->smallImageText) {
                    WriteObject assets(writer, "assets");
                    WriteOptionalString(writer, "large_image", presence->largeImageKey);
                    WriteOptionalString(writer, "large_text", presence->largeImageText);
                    WriteOptionalString(writer, "small_image", presence->smallImageKey);
                    WriteOptionalString(writer, "small_text", presence->smallImageText);
                }

                if (presence->partyId || presence->partySize || presence->partyMax) {
                    WriteObject party(writer, "party");
                    WriteOptionalString(writer, "id", presence->partyId);
                    if (presence->partySize) {
                        WriteArray size(writer, "size");
                        writer.Int(presence->partySize);
                        if (0 < presence->partyMax) {
                            writer.Int(presence->partyMax);
                        }
                    }
                }

                if (presence->matchSecret || presence->joinSecret || presence->spectateSecret) {
                    WriteObject secrets(writer, "secrets");
                    WriteOptionalString(writer, "match", presence->matchSecret);
                    WriteOptionalString(writer, "join", presence->joinSecret);
                    WriteOptionalString(writer, "spectate", presence->spectateSecret);
                }

                writer.Key("instance");
                writer.Bool(presence->instance != 0);
            }
        }
    }

    return writer.Size();
}

size_t JsonWriteHandshakeObj(char* dest, size_t maxLen, int version, const char* applicationId)
{
    JsonWriter writer(dest, maxLen);

    {
        WriteObject obj(writer);
        WriteKey(writer, "v");
        writer.Int(version);
        WriteKey(writer, "client_id");
        writer.String(applicationId);
    }

    return writer.Size();
}

size_t JsonWriteSubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName)
{
    JsonWriter writer(dest, maxLen);

    {
        WriteObject obj(writer);

        JsonWriteNonce(writer, nonce);

        WriteKey(writer, "cmd");
        writer.String("SUBSCRIBE");

        WriteKey(writer, "evt");
        writer.String(evtName);
    }

    return writer.Size();
}