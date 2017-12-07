
void mergeSortInt(Record** records, int len, int column){			
	if(len < 2){
		return;
	}else{
		int i = 0;
		int mid = len/2;
		Record* left[mid];				//size of left sub arr
		Record* right[len - mid];			//size of right sub arr
		for(i = 0; i < mid; i++){		//fill in sub arrays
			left[i] = records[i];
		}
		for(i = mid; i < len; i++){
			right[i - mid] = records[i];
		}
		mergeSortInt(left,mid, column);		//seperates all of left sides into smallest subb arr
		mergeSortInt(right, len - mid, column); //seperates all of right sides into smallest subb arr
		//at this point we have all basic sub array and this func combines them into main arr
		mergeInt(left, right, records, mid, len-mid, column); 	
	}
}

void mergeInt(Record** left, Record** right, Record** records, int lenL,int lenR, int column){
	int i = 0;
	int j = 0;
	int k = 0;
	while(i < lenL && j < lenR){
		
		Field *leftField = &(left[i]->fields[column]);
		Field *rightField = &(right[j]->fields[column]);
		
		// check type data to make the correct compare
		// IF one is NULL
		if (leftField->type == TYPE_NULL){
			records[k] = left[i];
			i++;
		}
		else if (rightField->type == TYPE_NULL){
			records[k] = right[j];
			j++;
		}
		// STRING compare
		else if (leftField->type == TYPE_STRING && rightField->type == TYPE_STRING){
			if(strcasecmp(leftField->data, rightField->data) < 0){
				records[k] = left[i];
				i++;
			} else if (strcasecmp(leftField->data, rightField->data) > 0){
				records[k] = right[j];
				j++;
			} else {
				// if equal
				if(strcmp(leftField->data, rightField->data) <= 0){
					records[k] = left[i];
					i++;
				} else {
					records[k] = right[j];
					j++;
				}
			}
		}
		// NUMERIC COMPARE
		else if (leftField->type == TYPE_NUMERIC && rightField->type == TYPE_NUMERIC){
			if(atof(leftField->data) <= atof(rightField->data)){
				records[k] = left[i];
				i++;
			}
			else{
				records[k] = right[j];
				j++;
			}
		} else {
			// type MISMATCH - compare as strings
			if(strcasecmp(leftField->data, rightField->data) < 0){
				records[k] = left[i];
				i++;
			} else if (strcasecmp(leftField->data, rightField->data) > 0){
				records[k] = right[j];
				j++;
			} else {
				// if equal
				if(strcmp(leftField->data, rightField->data) <= 0){
					records[k] = left[i];
					i++;
				} else {
					records[k] = right[j];
					j++;
				}
			}
		}

		k++;
	}
	while(i < lenL){
		records[k] = left[i];
		k++;
		i++;
	}
	while(j < lenR){
		records[k] = right[j];
		k++;
		j++;
	}
}