#include <slib.h>

using namespace slib;

String Encrypt(const StringView& plain, const StringView& password, const StringView& iv)
{
	sl_uint8 key[32];
	PBKDF2_HMAC_SHA256::generateKey(password.getData(), password.getLength(), sl_null, 0, 1, key, sizeof(key));
	AES aes;
	aes.setKey(key, sizeof(key));
	char hashIV[16];
	MD5::hash(iv, hashIV);
	return String::makeHexString(aes.encrypt_CBC_PKCS7Padding(hashIV, plain.getData(), plain.getLength()));
}

String Decrypt(const StringView& strCipher, const StringView& password, const StringView& iv)
{
	sl_uint8 key[32];
	PBKDF2_HMAC_SHA256::generateKey(password.getData(), password.getLength(), sl_null, 0, 1, key, sizeof(key));
	AES aes;
	aes.setKey(key, sizeof(key));
	char hashIV[16];
	MD5::hash(iv, hashIV);
	Memory cipher = strCipher.parseHexString();
	return String::fromUtf8(aes.decrypt_CBC_PKCS7Padding(hashIV, cipher.getData(), cipher.getSize()));
}

int main(int argc, const char * argv[])
{
	StringView iv = "abc";
	StringView key = "aaaa";
	String encrypt = Encrypt("This string is used to check AES.", key, iv);
	String decrypt = Decrypt(encrypt, key, iv);
	Println("Encrypt: %s", encrypt);
	Println("Decrypt: %s", decrypt);
	return 0;
}
