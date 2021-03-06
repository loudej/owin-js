#include "owin.h"

OwinJavaScriptConverter::OwinJavaScriptConverter()
{
	this->Buffers = gcnew List<cli::array<byte>^>();
}

System::Object^ OwinJavaScriptConverter::Deserialize(
        IDictionary<System::String^, System::Object^>^ dictionary, 
        System::Type^ type, 
        JavaScriptSerializer^ serializer)
{
	throw gcnew System::NotImplementedException();
}

IDictionary<System::String^, System::Object^>^ OwinJavaScriptConverter::Serialize(
        System::Object^ obj, 
        JavaScriptSerializer^ serializer)
{
	cli::array<byte>^ buffer = (cli::array<byte>^)obj;
	int bufferId = this->Buffers->Count;
	this->Buffers->Add(buffer);
	Dictionary<System::String^,System::Object^>^ result = gcnew Dictionary<System::String^,System::Object^>();
	result->Add("_owin_node_buffer_id", bufferId);
	return result;
} 

Handle<v8::Value> OwinJavaScriptConverter::FixupBuffers(Handle<v8::Value> data)
{
	HandleScope scope;

	if (data->IsArray())
	{
        Handle<v8::Array> jsarray = Handle<v8::Array>::Cast(data);
        for (unsigned int i = 0; i < jsarray->Length(); i++)
        {
            Handle<v8::Value> value = jsarray->Get(i);
            jsarray->Set(i, this->FixupBuffers(value));
        }
	}
	else if (data->IsObject())
	{
		Handle<v8::Object> jsobject = data->ToObject();
    	Handle<v8::Value> bufferId = jsobject->Get(v8::String::NewSymbol("_owin_node_buffer_id"));
    	if (bufferId->IsInt32())
    	{
    		cli::array<byte>^ buffer = this->Buffers[bufferId->Int32Value()];
	        pin_ptr<unsigned char> pinnedBuffer = &buffer[0];
	        node::Buffer* slowBuffer = node::Buffer::New(buffer->Length);
	        memcpy(node::Buffer::Data(slowBuffer), pinnedBuffer, buffer->Length);
	        Handle<v8::Value> args[] = { 
	            slowBuffer->handle_, 
	            v8::Integer::New(buffer->Length), 
	            v8::Integer::New(0) 
	        };
	        Handle<v8::Object> fastBuffer = bufferConstructor->NewInstance(3, args);    
	        return scope.Close(fastBuffer);		
    	}
    	else 
    	{
	        Handle<v8::Array> propertyNames = jsobject->GetPropertyNames();
	        for (unsigned int i = 0; i < propertyNames->Length(); i++)
	        {
	            Handle<v8::String> name = Handle<v8::String>::Cast(propertyNames->Get(i));
	            jsobject->Set(name, this->FixupBuffers(jsobject->Get(name)));
	        }
    	}
	}

	return scope.Close(data);
}
