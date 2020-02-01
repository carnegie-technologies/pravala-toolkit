/*
 *  Copyright 2019 Carnegie Technologies
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package com.pravala.utilities;

import java.lang.reflect.Field;

/**
 * A class that contains utility method(s) for comparing objects
 */
public class ObjectUtils
{
    /**
     * Compare two objects while checking for nulls.
     * @param a Any object.
     * @param b Any object.
     * @return True if a equals b while checking for nulls. Returns true if both are null.
     */
    public static boolean equals ( Object a, Object b )
    {
        if ( a == null )
        {
            return ( b == null );
        }

        return a.equals ( b );
    }

    /**
     * Use reflection to recursively search for the first field with the given name
     * in the given object's class or super classes.
     * @param obj The object to look for the field in.
     * @param fieldName The name of the field.
     * @return The field on success; Null on failure.
     */
    public static Field getFirstDeclaredField ( Object obj, String fieldName )
    {
        if ( obj == null || fieldName == null )
        {
            return null;
        }

        Field field = null;
        Class currClass = obj.getClass();

        while ( field == null && currClass != null )
        {
            try
            {
                field = currClass.getDeclaredField ( fieldName );
            }
            catch ( Exception e )
            {
            }

            currClass = currClass.getSuperclass();
        }

        return field;
    }
}
