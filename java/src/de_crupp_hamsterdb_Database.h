/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class de_crupp_hamsterdb_Database */

#ifndef _Included_de_crupp_hamsterdb_Database
#define _Included_de_crupp_hamsterdb_Database
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_get_version
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1get_1version
  (JNIEnv *, jclass, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_get_license
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_de_crupp_hamsterdb_Database_ham_1get_1license
  (JNIEnv *, jclass, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_set_errhandler
 * Signature: (Lde/crupp/hamsterdb/ErrorHandler;)V
 */
JNIEXPORT void JNICALL Java_de_crupp_hamsterdb_Database_ham_1set_1errhandler
  (JNIEnv *, jclass, jobject);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_get_error
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1get_1error
  (JNIEnv *, jobject, jlong);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_set_compare_func
 * Signature: (JLde/crupp/hamsterdb/CompareCallback;)V
 */
JNIEXPORT void JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1set_1compare_1func
  (JNIEnv *, jobject, jlong, jobject);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_set_prefix_compare_func
 * Signature: (JLde/crupp/hamsterdb/PrefixCompareCallback;)V
 */
JNIEXPORT void JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1set_1prefix_1compare_1func
  (JNIEnv *, jobject, jlong, jobject);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_find
 * Signature: (JJ[BI)[B
 */
JNIEXPORT jbyteArray JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1find
  (JNIEnv *, jobject, jlong, jlong, jbyteArray, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_get_parameters
 * Signature: (J[Lde/crupp/hamsterdb/Parameter;)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1get_1parameters
  (JNIEnv *, jobject, jlong, jobjectArray);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_insert
 * Signature: (JJ[B[BI)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1insert
  (JNIEnv *, jobject, jlong, jlong, jbyteArray, jbyteArray, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_erase
 * Signature: (JJ[BI)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1erase
  (JNIEnv *, jobject, jlong, jlong, jbyteArray, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_get_key_count
 * Signature: (JJI)J
 */
JNIEXPORT jlong JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1get_1key_1count
  (JNIEnv *, jobject, jlong, jlong, jint);

/*
 * Class:     de_crupp_hamsterdb_Database
 * Method:    ham_db_close
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_de_crupp_hamsterdb_Database_ham_1db_1close
  (JNIEnv *, jobject, jlong, jint);

#ifdef __cplusplus
}
#endif
#endif
