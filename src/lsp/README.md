This is the Language Server Protocol implementation for OpenSCAD

It does implement more than just a run-of-the-mill languageserver, since it provides a certain level of GUI-control.


# General Structure

The `LanguageServerInterface` describes the interface between Qt-OpenSCAD and the language server implementation.
The interface will start a `ConnectionHandler` to handle incoming `Connection`s.

Incoming messages are decoded in the `ConnectionHandler`, where the mapping between the LSP-methods and local types is declared in `ConnectionHandler::register_messages` (defined in decoding.cc - for C++ scoping reasons).

One method can only be mapped to one type - and generally vice versa!
After a message has been decoded into its corresponding type it's `::process` method will be called.

According to the Language Server Protocol any Request Message _must_ return a response, which is done via `Connection::send` - an argument to the `process()` method.

Requests can be sent from outside of the message structure using `ConnectionHandler::send` - which will select the oldest of the active connections to send the request to.

Responses are handled using the given callback.
The Callback can be set to `Connection::no_reponse_expected`, which will decode the response and handle only returned error messages by printing them out.

# Adding Messages

Adding a message type involves several steps - and as a note for myself in a year.

## Define message

The message is defined in `messages.h` by inheriting from `RequestMessage` if it is a request, or `ResponseResult` if it is the result of a given message according to this template:

```cpp
MESSAGE_CLASS(DidOpenTextDocument) : public RequestMessage {
    MAKE_DECODEABLE;

    TextDocumentItem textDocument;
    virtual void process(Connection *, project *, const RequestId &id);
};
```

The fields defined in the `RequestMessage` are the ones listed for the JSON-RPC `params` field.
A `RequestMessage` has to have a `process()` method.

Standard types for the language server protocol are defined in `lsp.h`, these can - and should - be used freely.

The usable templates are:

 * `OptionalType<>`: (an typedef to std::optional or boost::optional depending on availability) The field is not required to be present.
 * `std::vector<>`: to implement JSON arrays.

## Define Decoding

In order for a message to be encoded and decoded, `decode_env::declare_field<>` has to be template overloaded for the given type.
Missing this will lead to an interesting printf error-hack hinting that this step has been forgotten.

The decoding is added to `decoding.cc` near the end, above the other `decode_env` implementations:

```cpp
template<>
bool decode_env::declare_field(JSONObject &object, DidOpenTextDocument &target, const FieldNameType &) {
    declare_field(object, target.textDocument, "textDocument");
    return true;
}
```

There is no difference between reading and writing - this magic is contained in the `decode_env`.

The overloaded types have to be defined differently - they have the same parameters.

 * `OptionalType` has to be defined using `declare_field_optional`
 * `std::vector` has to be defined using `declare_field_array`

To encode a new child object in the place of the current field, you have to use `start_object(parent, fieldname)` giving the parent object (the one given to `declare_field`) and the field name in where the child should be written/read.
You should only use `start_object` in the start of an object if the type is defined as object - not outside! (except when you fought a full day against it!)

## Add Mapping

For `RequestMessage` types you have to add the mapping to `ConnectionHandler::register_messages` by invocing the `MAP` macro:

```cpp
    MAP("textDocument/didOpen", DidOpenTextDocument);
```

As you can see - trivialy.

## Add implementation

Now finally to the fun part: The actual handling of the message.

These are for the moment defined in `messages.cc`, but here the scoping is not an issue.

Every Request has to deliver a response using the `conn->send` magic. The state of the project is to be handled in the `project` also given to the handler method.

Feel free to change and mishandle the `project_t` in `project.h` at your own leisure.


# Connecting with Qt

The connection is done in `language_server_interface.h`, mapping between Qt signals and requests and responses - you have to correctly implement the messages correctly to `emit` the signals on the language server interface.